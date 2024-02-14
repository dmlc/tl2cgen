/*!
 * Copyright (c) 2023-2024 by Contributors
 * \file annotator.cc
 * \author Hyunsu Cho
 * \brief Branch annotation tools
 */

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <tl2cgen/annotator.h>
#include <tl2cgen/detail/math_funcs.h>
#include <tl2cgen/detail/operator_comp.h>
#include <tl2cgen/detail/threading_utils/parallel_for.h>
#include <tl2cgen/logging.h>
#include <treelite/enum/operator.h>
#include <treelite/enum/tree_node_type.h>
#include <treelite/tree.h>

#include <cstdint>
#include <limits>
#include <thread>
#include <variant>

namespace {

using tl2cgen::detail::threading_utils::ThreadConfig;

template <typename ElementType>
union Entry {
  int missing;
  ElementType fvalue;
};

template <typename ElementType, typename ThresholdType, typename LeafOutputType>
void Traverse_(treelite::Tree<ThresholdType, LeafOutputType> const& tree,
    Entry<ElementType> const* data, int nid, std::uint64_t* out_counts) {
  ++out_counts[nid];
  if (!tree.IsLeaf(nid)) {
    unsigned const split_index = tree.SplitIndex(nid);

    if (data[split_index].missing == -1) {
      Traverse_(tree, data, tree.DefaultChild(nid), out_counts);
    } else {
      bool result = true;
      if (tree.NodeType(nid) == treelite::TreeNodeType::kNumericalTestNode) {
        ThresholdType const threshold = tree.Threshold(nid);
        treelite::Operator const op = tree.ComparisonOp(nid);
        auto const fvalue = static_cast<ElementType>(data[split_index].fvalue);
        result = tl2cgen::detail::CompareWithOp(fvalue, op, threshold);
      } else {
        auto const fvalue = data[split_index].fvalue;
        auto const category_list = tree.CategoryList(nid);
        result = (std::binary_search(
            category_list.begin(), category_list.end(), static_cast<std::uint32_t>(fvalue)));
        if (tree.CategoryListRightChild(nid)) {
          result = !result;
        }
      }
      if (result) {  // left child
        Traverse_(tree, data, tree.LeftChild(nid), out_counts);
      } else {  // right child
        Traverse_(tree, data, tree.RightChild(nid), out_counts);
      }
    }
  }
}

template <typename ElementType, typename ThresholdType, typename LeafOutputType>
void Traverse(treelite::Tree<ThresholdType, LeafOutputType> const& tree,
    Entry<ElementType> const* data, std::uint64_t* out_counts) {
  Traverse_(tree, data, 0, out_counts);
}

}  // anonymous namespace

namespace tl2cgen {
namespace detail {

template <typename DMatrixT>
class ComputeBranchLooper {};

template <typename ElementType>
class ComputeBranchLooper<DenseDMatrix<ElementType>> {
 public:
  template <typename ThresholdType, typename LeafOutputType>
  static void Loop(treelite::ModelPreset<ThresholdType, LeafOutputType> const& model_preset,
      DenseDMatrix<ElementType> const& dmat, std::uint64_t rbegin, std::uint64_t rend,
      ThreadConfig const& thread_config, std::uint64_t const* count_row_ptr,
      std::uint64_t* counts_tloc) {
    std::vector<Entry<ElementType>> inst(thread_config.nthread * dmat.num_col_, {-1});
    std::uint64_t ntree = model_preset.trees.size();
    TL2CGEN_CHECK_LE(rbegin, rend);
    std::uint64_t num_col = dmat.num_col_;
    ElementType missing_value = dmat.missing_value_;
    bool nan_missing = tl2cgen::detail::math::CheckNAN(missing_value);
    auto sched = tl2cgen::detail::threading_utils::ParallelSchedule::Static();
    tl2cgen::detail::threading_utils::ParallelFor(
        rbegin, rend, thread_config, sched, [&](std::uint64_t rid, int thread_id) {
          const ElementType* row = &dmat.data_[rid * num_col];
          const std::uint64_t off = dmat.num_col_ * thread_id;
          const std::uint64_t off2 = count_row_ptr[ntree] * thread_id;
          for (std::uint64_t j = 0; j < num_col; ++j) {
            if (tl2cgen::detail::math::CheckNAN(row[j])) {
              TL2CGEN_CHECK(nan_missing) << "The missing_value argument must be set to NaN if "
                                            "there is any NaN in the matrix.";
            } else if (nan_missing || row[j] != missing_value) {
              inst[off + j].fvalue = row[j];
            }
          }
          for (std::uint64_t tree_id = 0; tree_id < ntree; ++tree_id) {
            Traverse(model_preset.trees[tree_id], &inst[off],
                &counts_tloc[off2 + count_row_ptr[tree_id]]);
          }
          for (std::uint64_t j = 0; j < num_col; ++j) {
            inst[off + j].missing = -1;
          }
        });
  }
};

template <typename ElementType>
class ComputeBranchLooper<CSRDMatrix<ElementType>> {
 public:
  template <typename ThresholdType, typename LeafOutputType>
  static void Loop(treelite::ModelPreset<ThresholdType, LeafOutputType> const& model_preset,
      CSRDMatrix<ElementType> const dmat, std::uint64_t rbegin, std::uint64_t rend,
      ThreadConfig const& thread_config, std::uint64_t const* count_row_ptr,
      std::uint64_t* counts_tloc) {
    std::vector<Entry<ElementType>> inst(thread_config.nthread * dmat.num_col_, {-1});
    std::uint64_t ntree = model_preset.trees.size();
    TL2CGEN_CHECK_LE(rbegin, rend);
    auto sched = tl2cgen::detail::threading_utils::ParallelSchedule::Static();
    tl2cgen::detail::threading_utils::ParallelFor(
        rbegin, rend, thread_config, sched, [&](std::uint64_t rid, int thread_id) {
          const std::uint64_t off = dmat.num_col_ * thread_id;
          const std::uint64_t off2 = count_row_ptr[ntree] * thread_id;
          const std::uint64_t ibegin = dmat.row_ptr_[rid];
          const std::uint64_t iend = dmat.row_ptr_[rid + 1];
          for (std::uint64_t i = ibegin; i < iend; ++i) {
            inst[off + dmat.col_ind_[i]].fvalue = dmat.data_[i];
          }
          for (std::uint64_t tree_id = 0; tree_id < ntree; ++tree_id) {
            Traverse(model_preset.trees[tree_id], &inst[off],
                &counts_tloc[off2 + count_row_ptr[tree_id]]);
          }
          for (std::uint64_t i = ibegin; i < iend; ++i) {
            inst[off + dmat.col_ind_[i]].missing = -1;
          }
        });
  }
};

template <typename ThresholdType, typename LeafOutputType>
inline void ComputeBranchLoop(treelite::ModelPreset<ThresholdType, LeafOutputType> const& model,
    tl2cgen::DMatrix const* dmat, std::uint64_t rbegin, std::uint64_t rend,
    ThreadConfig const& thread_config, std::uint64_t const* count_row_ptr,
    std::uint64_t* counts_tloc) {
  std::visit(
      [&model, rbegin, rend, &thread_config, count_row_ptr, counts_tloc](auto&& concrete_dmat) {
        using DMatrixType = std::remove_const_t<std::remove_reference_t<decltype(concrete_dmat)>>;
        ComputeBranchLooper<DMatrixType>::Loop(
            model, concrete_dmat, rbegin, rend, thread_config, count_row_ptr, counts_tloc);
      },
      dmat->variant_);
}

template <typename ThresholdType, typename LeafOutputType>
inline void AnnotateImpl(treelite::ModelPreset<ThresholdType, LeafOutputType> const& model_preset,
    tl2cgen::DMatrix const* dmat, int nthread, int verbose,
    std::vector<std::vector<std::uint64_t>>* out_counts) {
  std::vector<std::uint64_t> new_counts;
  std::vector<std::uint64_t> counts_tloc;
  std::vector<std::uint64_t> count_row_ptr;

  count_row_ptr = {0};
  std::uint64_t const ntree = model_preset.trees.size();
  ThreadConfig thread_config = detail::threading_utils::ConfigureThreadConfig(nthread);
  for (treelite::Tree<ThresholdType, LeafOutputType> const& tree : model_preset.trees) {
    count_row_ptr.push_back(count_row_ptr.back() + tree.num_nodes);
  }
  new_counts.resize(count_row_ptr[ntree], 0);
  counts_tloc.resize(count_row_ptr[ntree] * thread_config.nthread, 0);

  std::uint64_t const num_row = dmat->GetNumRow();
  std::uint64_t const pstep = (num_row + 19) / 20;
  // Interval to display progress
  for (std::uint64_t rbegin = 0; rbegin < num_row; rbegin += pstep) {
    std::uint64_t const rend = std::min(rbegin + pstep, num_row);
    ComputeBranchLoop(
        model_preset, dmat, rbegin, rend, thread_config, &count_row_ptr[0], &counts_tloc[0]);
    if (verbose > 0) {
      TL2CGEN_LOG(INFO) << rend << " of " << num_row << " rows processed";
    }
  }

  // perform reduction on counts
  for (std::uint32_t tid = 0; tid < thread_config.nthread; ++tid) {
    std::uint64_t const off = count_row_ptr[ntree] * tid;
    for (std::uint64_t i = 0; i < count_row_ptr[ntree]; ++i) {
      new_counts[i] += counts_tloc[off + i];
    }
  }

  // change layout of counts
  std::vector<std::vector<std::uint64_t>>& counts = *out_counts;
  for (std::uint64_t i = 0; i < ntree; ++i) {
    counts.emplace_back(
        new_counts.begin() + count_row_ptr[i], new_counts.begin() + count_row_ptr[i + 1]);
  }
}

}  // namespace detail

void BranchAnnotator::Annotate(
    treelite::Model const& model, DMatrix const* dmat, int nthread, int verbose) {
  TL2CGEN_CHECK(dmat) << "Dangling data matrix reference detected";

  std::visit(
      [this, dmat, nthread, verbose](auto&& model_preset) {
        detail::AnnotateImpl(model_preset, dmat, nthread, verbose, &this->counts_);
      },
      model.variant_);
}

void BranchAnnotator::Load(std::istream& fi) {
  rapidjson::IStreamWrapper is(fi);

  rapidjson::Document doc;
  doc.ParseStream(is);

  std::string err_msg = "JSON file must contain a list of lists of integers";
  TL2CGEN_CHECK(doc.IsArray()) << err_msg;
  counts_.clear();
  for (auto const& node_cnt : doc.GetArray()) {
    TL2CGEN_CHECK(node_cnt.IsArray()) << err_msg;
    counts_.emplace_back();
    for (auto const& e : node_cnt.GetArray()) {
      counts_.back().push_back(e.GetUint64());
    }
  }
}

void BranchAnnotator::Save(std::ostream& fo) const {
  rapidjson::OStreamWrapper os(fo);
  rapidjson::Writer<rapidjson::OStreamWrapper> writer(os);

  writer.StartArray();
  for (auto const& node_cnt : counts_) {
    writer.StartArray();
    for (auto e : node_cnt) {
      writer.Uint64(e);
    }
    writer.EndArray();
  }
  writer.EndArray();
}

}  // namespace tl2cgen
