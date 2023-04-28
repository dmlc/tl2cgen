/*!
 * Copyright (c) 2023 by Contributors
 * \file annotator.h
 * \author Hyunsu Cho
 * \brief Branch annotation tools
 */
#ifndef TL2CGEN_ANNOTATOR_H_
#define TL2CGEN_ANNOTATOR_H_

#include <tl2cgen/data_matrix.h>

#include <cstdint>
#include <cstdio>
#include <istream>
#include <ostream>
#include <vector>

namespace treelite {

class Model;

}

namespace tl2cgen {

/*! \brief branch annotator class */
class BranchAnnotator {
 public:
  /*!
   * \brief Annotate branches in a given model using frequency patterns in the
   *        training data. The annotation can be accessed through Get() method.
   * \param model Tree ensemble model
   * \param dmat Training data matrix
   * \param nthread Number of threads to use
   * \param verbose Whether to produce extra messages
   */
  void Annotate(
      treelite::Model const& model, tl2cgen::DMatrix const* dmat, int nthread, int verbose);
  /*!
   * \brief Load branch annotation from a JSON file
   * \param fi Input stream
   */
  void Load(std::istream& fi);
  /*!
   * \brief Save branch annotation to a JSON file
   * \param fo Output stream
   */
  void Save(std::ostream& fo) const;
  /*!
   * \brief Fetch branch annotation.
   * Usage example:
   * \code
   *   Annotator annotator
   *   annotator.Load(fi);  // load from a stream
   *   std::vector<std::vector<size_t>> annot = annotator.Get();
   *   // access the frequency count for a specific node in a tree
   *   TL2CGEN_LOG(INFO) << "Tree " << tree_id << ", Node " << node_id << ": "
   *                     << annot[tree_id][node_id];
   * \endcode
   * \return Branch annotation in 2D vector
   */
  inline std::vector<std::vector<std::uint64_t>> Get() const {
    return counts_;
  }

 private:
  std::vector<std::vector<std::uint64_t>> counts_;
};

}  // namespace tl2cgen

#endif  // TL2CGEN_ANNOTATOR_H_
