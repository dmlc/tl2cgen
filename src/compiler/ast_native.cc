/*!
 * Copyright (c) 2023 by Contributors
 * \file ast_native.cc
 * \brief C code generator
 * \author Hyunsu Cho
 */
#include <fmt/format.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <tl2cgen/annotator.h>
#include <tl2cgen/compiler.h>
#include <tl2cgen/compiler_param.h>
#include <tl2cgen/detail/compiler/ast/builder.h>
#include <tl2cgen/detail/compiler/ast_native.h>
#include <tl2cgen/detail/compiler/pred_transform.h>
#include <tl2cgen/detail/compiler/templates/code_folder_template.h>
#include <tl2cgen/detail/compiler/templates/header_template.h>
#include <tl2cgen/detail/compiler/templates/main_template.h>
#include <tl2cgen/detail/compiler/templates/qnode_template.h>
#include <tl2cgen/detail/compiler/templates/typeinfo.h>
#include <tl2cgen/detail/compiler/util/code_folding_util.h>
#include <tl2cgen/detail/compiler/util/format_util.h>
#include <treelite/tree.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <queue>
#include <unordered_map>

#if defined(_MSC_VER) || defined(_WIN32)
#define DLLEXPORT_KEYWORD "__declspec(dllexport) "
#else
#define DLLEXPORT_KEYWORD ""
#endif

using namespace fmt::literals;

namespace tl2cgen::compiler::detail {

class ASTNativeCompilerImpl {
 public:
  explicit ASTNativeCompilerImpl(CompilerParam const& param) : param_(param) {}

  template <typename ThresholdType, typename LeafOutputType>
  CompiledModel CompileImpl(treelite::ModelImpl<ThresholdType, LeafOutputType> const& model) {
    CompiledModel cm;

    TL2CGEN_CHECK(model.task_type != treelite::TaskType::kMultiClfCategLeaf)
        << "Model task type unsupported by ASTNativeCompiler";
    TL2CGEN_CHECK(model.task_param.output_type == treelite::TaskParam::OutputType::kFloat)
        << "ASTNativeCompiler only supports models with float output";

    num_feature_ = model.num_feature;
    task_type_ = model.task_type;
    task_param_ = model.task_param;
    pred_transform_ = model.param.pred_transform;
    sigmoid_alpha_ = model.param.sigmoid_alpha;
    ratio_c_ = model.param.ratio_c;
    global_bias_ = model.param.global_bias;
    files_.clear();

    ast::ASTBuilder<ThresholdType, LeafOutputType> builder;
    builder.BuildAST(model);
    if (builder.FoldCode(param_.code_folding_req) || param_.quantize > 0) {
      // is_categorical[i] : is i-th feature categorical?
      array_is_categorical_ = RenderIsCategoricalArray(builder.GenerateIsCategoricalArray());
    }
    if (param_.annotate_in != "NULL") {
      BranchAnnotator annotator;
      std::ifstream fi(param_.annotate_in.c_str());
      annotator.Load(fi);
      auto const annotation = annotator.Get();
      builder.LoadDataCounts(annotation);
      TL2CGEN_LOG(INFO) << "Loading node frequencies from `" << param_.annotate_in << "'";
    }
    builder.Split(param_.parallel_comp);
    if (param_.quantize > 0) {
      builder.QuantizeThresholds();
    }

    {
      char const* destfile = getenv("TL2CGEN_DUMP_AST");
      if (destfile) {
        std::ofstream os(destfile);
        os << builder.GetDump() << std::endl;
      }
    }

    WalkAST<ThresholdType, LeafOutputType>(builder.GetRootNode(), "main.c", 0);
    if (files_.count("arrays.c") > 0) {
      PrependToBuffer("arrays.c", "#include \"header.h\"\n", 0);
    }

    {
      /* write recipe.json */
      rapidjson::StringBuffer os;
      rapidjson::Writer<rapidjson::StringBuffer> writer(os);

      writer.StartObject();
      writer.Key("target");
      writer.String(param_.native_lib_name.data(), param_.native_lib_name.size());
      writer.Key("sources");
      writer.StartArray();
      for (auto const& kv : files_) {
        if (kv.first.compare(kv.first.length() - 2, 2, ".c") == 0) {
          const std::size_t line_count
              = std::count(kv.second.content.begin(), kv.second.content.end(), '\n');
          writer.StartObject();
          writer.Key("name");
          std::string name = kv.first.substr(0, kv.first.length() - 2);
          writer.String(name.data(), name.size());
          writer.Key("length");
          writer.Uint64(line_count);
          writer.EndObject();
        }
      }
      writer.EndArray();
      writer.EndObject();

      files_["recipe.json"] = CompiledModel::FileEntry(os.GetString());
    }
    cm.files = std::move(files_);
    return cm;
  }

  CompiledModel Compile(treelite::Model const& model) {
    TL2CGEN_CHECK(model.GetLeafOutputType() != treelite::TypeInfo::kUInt32)
        << "Integer leaf outputs not yet supported";
    this->pred_tranform_func_ = PredTransformFunction(model);
    return model.Dispatch(
        [this](auto const& model_handle) { return this->CompileImpl(model_handle); });
  }

  CompilerParam QueryParam() const {
    return param_;
  }

 private:
  CompilerParam param_;
  int num_feature_;
  treelite::TaskType task_type_;
  treelite::TaskParam task_param_;
  std::string pred_transform_;
  float sigmoid_alpha_;
  float ratio_c_;
  float global_bias_;
  std::string pred_tranform_func_;
  std::string array_is_categorical_;
  std::unordered_map<std::string, CompiledModel::FileEntry> files_;

  template <typename ThresholdType, typename LeafOutputType>
  void WalkAST(ast::ASTNode const* node, std::string const& dest, std::size_t indent) {
    ast::MainNode const* t1;
    ast::AccumulatorContextNode const* t2;
    ast::ConditionNode const* t3;
    ast::OutputNode<LeafOutputType> const* t4;
    ast::TranslationUnitNode const* t5;
    ast::QuantizerNode<ThresholdType> const* t6;
    ast::CodeFolderNode const* t7;
    if ((t1 = dynamic_cast<ast::MainNode const*>(node))) {
      HandleMainNode<ThresholdType, LeafOutputType>(t1, dest, indent);
    } else if ((t2 = dynamic_cast<ast::AccumulatorContextNode const*>(node))) {
      HandleACNode<ThresholdType, LeafOutputType>(t2, dest, indent);
    } else if ((t3 = dynamic_cast<ast::ConditionNode const*>(node))) {
      HandleCondNode<ThresholdType, LeafOutputType>(t3, dest, indent);
    } else if ((t4 = dynamic_cast<ast::OutputNode<LeafOutputType> const*>(node))) {
      HandleOutputNode<ThresholdType, LeafOutputType>(t4, dest, indent);
    } else if ((t5 = dynamic_cast<ast::TranslationUnitNode const*>(node))) {
      HandleTUNode<ThresholdType, LeafOutputType>(t5, dest, indent);
    } else if ((t6 = dynamic_cast<ast::QuantizerNode<ThresholdType> const*>(node))) {
      HandleQNode<ThresholdType, LeafOutputType>(t6, dest, indent);
    } else if ((t7 = dynamic_cast<ast::CodeFolderNode const*>(node))) {
      HandleCodeFolderNode<ThresholdType, LeafOutputType>(t7, dest, indent);
    } else {
      TL2CGEN_LOG(FATAL) << "Unrecognized AST node type";
    }
  }

  // append content to a given buffer, with given level of indentation
  inline void AppendToBuffer(
      std::string const& dest, std::string const& content, std::size_t indent) {
    files_[dest].content += util::IndentMultiLineString(content, indent);
  }

  // prepend content to a given buffer, with given level of indentation
  inline void PrependToBuffer(
      std::string const& dest, std::string const& content, std::size_t indent) {
    files_[dest].content = util::IndentMultiLineString(content, indent) + files_[dest].content;
  }

  template <typename ThresholdType, typename LeafOutputType>
  void HandleMainNode(ast::MainNode const* node, std::string const& dest, std::size_t indent) {
    const std::string threshold_type
        = templates::TypeInfoToCTypeString(treelite::TypeToInfo<ThresholdType>());
    const std::string leaf_output_type
        = templates::TypeInfoToCTypeString(treelite::TypeToInfo<LeafOutputType>());
    const std::string predict_function_signature
        = (task_param_.num_class > 1)
              ? fmt::format(
                  "size_t predict_multiclass(union Entry* data, int pred_margin, {}* result)",
                  leaf_output_type)
              : fmt::format("{} predict(union Entry* data, int pred_margin)", leaf_output_type);

    if (!array_is_categorical_.empty()) {
      array_is_categorical_
          = fmt::format("const unsigned char is_categorical[] = {{\n{}\n}}", array_is_categorical_);
    }

    const std::string query_functions_definition = fmt::format(
        templates::query_functions_definition_template, "num_class"_a = task_param_.num_class,
        "num_feature"_a = num_feature_, "pred_transform"_a = pred_transform_,
        "sigmoid_alpha"_a = sigmoid_alpha_, "ratio_c"_a = ratio_c_, "global_bias"_a = global_bias_,
        "threshold_type_str"_a = treelite::TypeInfoToString(treelite::TypeToInfo<ThresholdType>()),
        "leaf_output_type_str"_a
        = treelite::TypeInfoToString(treelite::TypeToInfo<LeafOutputType>()));

    AppendToBuffer(dest,
        fmt::format(templates::main_start_template,
            "array_is_categorical"_a = array_is_categorical_,
            "query_functions_definition"_a = query_functions_definition,
            "pred_transform_function"_a = pred_tranform_func_,
            "predict_function_signature"_a = predict_function_signature),
        indent);
    const std::string query_functions_prototype = fmt::format(
        templates::query_functions_prototype_template, "dllexport"_a = DLLEXPORT_KEYWORD);
    AppendToBuffer("header.h",
        fmt::format(templates::header_template, "dllexport"_a = DLLEXPORT_KEYWORD,
            "predict_function_signature"_a = predict_function_signature,
            "query_functions_prototype"_a = query_functions_prototype,
            "threshold_type"_a = threshold_type,
            "threshold_type_Node"_a = (param_.quantize > 0 ? std::string("int") : threshold_type)),
        indent);

    TL2CGEN_CHECK_EQ(node->children.size(), 1);
    WalkAST<ThresholdType, LeafOutputType>(node->children[0], dest, indent + 2);

    std::string optional_average_field;
    if (node->average_result) {
      if (task_type_ == treelite::TaskType::kMultiClfGrovePerClass) {
        TL2CGEN_CHECK(task_param_.grove_per_class);
        TL2CGEN_CHECK_EQ(task_param_.leaf_vector_size, 1);
        TL2CGEN_CHECK_GT(task_param_.num_class, 1);
        TL2CGEN_CHECK_EQ(node->num_tree % task_param_.num_class, 0)
            << "Expected the number of trees to be divisible by the number of classes";
        int num_boosting_round = node->num_tree / static_cast<int>(task_param_.num_class);
        optional_average_field = fmt::format(" / {}", num_boosting_round);
      } else {
        TL2CGEN_CHECK(task_type_ == treelite::TaskType::kBinaryClfRegr
                      || task_type_ == treelite::TaskType::kMultiClfProbDistLeaf);
        TL2CGEN_CHECK_EQ(task_param_.num_class, task_param_.leaf_vector_size);
        TL2CGEN_CHECK(!task_param_.grove_per_class);
        optional_average_field = fmt::format(" / {}", node->num_tree);
      }
    }
    if (task_param_.num_class > 1) {
      AppendToBuffer(dest,
          fmt::format(templates::main_end_multiclass_template,
              "num_class"_a = task_param_.num_class,
              "optional_average_field"_a = optional_average_field,
              "global_bias"_a = util::ToStringHighPrecision(node->global_bias),
              "leaf_output_type"_a = leaf_output_type),
          indent);
    } else {
      AppendToBuffer(dest,
          fmt::format(templates::main_end_template,
              "optional_average_field"_a = optional_average_field,
              "global_bias"_a = util::ToStringHighPrecision(node->global_bias),
              "leaf_output_type"_a = leaf_output_type),
          indent);
    }
  }

  template <typename ThresholdType, typename LeafOutputType>
  void HandleACNode(
      ast::AccumulatorContextNode const* node, std::string const& dest, std::size_t indent) {
    const std::string leaf_output_type
        = templates::TypeInfoToCTypeString(treelite::TypeToInfo<LeafOutputType>());
    if (task_param_.num_class > 1) {
      AppendToBuffer(dest,
          fmt::format("{leaf_output_type} sum[{num_class}] = {{0}};\n"
                      "unsigned int tmp;\n"
                      "int nid, cond, fid;  /* used for folded subtrees */\n",
              "num_class"_a = task_param_.num_class, "leaf_output_type"_a = leaf_output_type),
          indent);
    } else {
      AppendToBuffer(dest,
          fmt::format("{leaf_output_type} sum = ({leaf_output_type})0;\n"
                      "unsigned int tmp;\n"
                      "int nid, cond, fid;  /* used for folded subtrees */\n",
              "leaf_output_type"_a = leaf_output_type),
          indent);
    }
    for (ast::ASTNode* child : node->children) {
      WalkAST<ThresholdType, LeafOutputType>(child, dest, indent);
    }
  }

  template <typename ThresholdType, typename LeafOutputType>
  void HandleCondNode(ast::ConditionNode const* node, std::string const& dest, std::size_t indent) {
    ast::NumericalConditionNode<ThresholdType> const* t;
    std::string condition_with_na_check;
    if ((t = dynamic_cast<ast::NumericalConditionNode<ThresholdType> const*>(node))) {
      /* numerical split */
      std::string condition = ExtractNumericalCondition(t);
      char const* condition_with_na_check_template
          = (node->default_left) ? "!(data[{split_index}].missing != -1) || ({condition})"
                                 : " (data[{split_index}].missing != -1) && ({condition})";
      condition_with_na_check = fmt::format(condition_with_na_check_template,
          "split_index"_a = node->split_index, "condition"_a = condition);
    } else { /* categorical split */
      auto const* t2 = dynamic_cast<ast::CategoricalConditionNode const*>(node);
      TL2CGEN_CHECK(t2);
      condition_with_na_check = ExtractCategoricalCondition(t2);
    }
    if (node->children[0]->data_count && node->children[1]->data_count) {
      const std::uint64_t left_freq = *node->children[0]->data_count;
      const std::uint64_t right_freq = *node->children[1]->data_count;
      condition_with_na_check = fmt::format(" {keyword}( {condition} ) ",
          "keyword"_a = ((left_freq > right_freq) ? "LIKELY" : "UNLIKELY"),
          "condition"_a = condition_with_na_check);
    }
    AppendToBuffer(dest, fmt::format("if ({}) {{\n", condition_with_na_check), indent);
    TL2CGEN_CHECK_EQ(node->children.size(), 2);
    WalkAST<ThresholdType, LeafOutputType>(node->children[0], dest, indent + 2);
    AppendToBuffer(dest, "} else {\n", indent);
    WalkAST<ThresholdType, LeafOutputType>(node->children[1], dest, indent + 2);
    AppendToBuffer(dest, "}\n", indent);
  }

  template <typename ThresholdType, typename LeafOutputType>
  void HandleOutputNode(
      ast::OutputNode<LeafOutputType> const* node, std::string const& dest, std::size_t indent) {
    AppendToBuffer(dest, RenderOutputStatement(node), indent);
    TL2CGEN_CHECK_EQ(node->children.size(), 0);
  }

  template <typename ThresholdType, typename LeafOutputType>
  void HandleTUNode(
      ast::TranslationUnitNode const* node, std::string const& dest, std::size_t indent) {
    int const unit_id = node->unit_id;
    const std::string new_file = fmt::format("tu{}.c", unit_id);
    const std::string leaf_output_type
        = templates::TypeInfoToCTypeString(treelite::TypeToInfo<LeafOutputType>());

    std::string unit_function_name, unit_function_signature, unit_function_call_signature;
    if (task_param_.num_class > 1) {
      unit_function_name = fmt::format("predict_margin_multiclass_unit{}", unit_id);
      unit_function_signature
          = fmt::format("void {function_name}(union Entry* data, {leaf_output_type}* result)",
              "function_name"_a = unit_function_name, "leaf_output_type"_a = leaf_output_type);
      unit_function_call_signature = fmt::format("{}(data, sum);\n", unit_function_name);
    } else {
      unit_function_name = fmt::format("predict_margin_unit{}", unit_id);
      unit_function_signature = fmt::format("{leaf_output_type} {function_name}(union Entry* data)",
          "function_name"_a = unit_function_name, "leaf_output_type"_a = leaf_output_type);
      unit_function_call_signature = fmt::format("sum += {}(data);\n", unit_function_name);
    }
    AppendToBuffer(dest, unit_function_call_signature, indent);
    AppendToBuffer(new_file,
        fmt::format("#include \"header.h\"\n"
                    "{} {{\n",
            unit_function_signature),
        0);
    TL2CGEN_CHECK_EQ(node->children.size(), 1);
    WalkAST<ThresholdType, LeafOutputType>(node->children[0], new_file, 2);
    if (task_param_.num_class > 1) {
      AppendToBuffer(new_file,
          fmt::format("  for (int i = 0; i < {num_class}; ++i) {{\n"
                      "    result[i] += sum[i];\n"
                      "  }}\n"
                      "}}\n",
              "num_class"_a = task_param_.num_class),
          0);
    } else {
      AppendToBuffer(new_file, "  return sum;\n}\n", 0);
    }
    AppendToBuffer("header.h", fmt::format("{};\n", unit_function_signature), 0);
  }

  template <typename ThresholdType, typename LeafOutputType>
  void HandleQNode(
      ast::QuantizerNode<ThresholdType> const* node, std::string const& dest, std::size_t indent) {
    const std::string threshold_type
        = templates::TypeInfoToCTypeString(treelite::TypeToInfo<ThresholdType>());
    /* render arrays needed to convert feature values into bin indices */
    std::string array_threshold, array_th_begin, array_th_len;
    // threshold[] : list of all thresholds that occur at least once in the
    //   ensemble model. For each feature, an ascending list of unique
    //   thresholds is generated. The range th_begin[i]:(th_begin[i]+th_len[i])
    //   of the threshold[] array stores the threshold list for feature i.
    std::size_t total_num_threshold;
    // to hold total number of (distinct) thresholds
    {
      util::ArrayFormatter formatter(80, 2);
      for (auto const& e : node->cut_pts) {
        // cut_pts had been generated in ASTBuilder::QuantizeThresholds
        // cut_pts[i][k] stores the k-th threshold of feature i.
        for (auto v : e) {
          formatter << v;
        }
      }
      array_threshold = formatter.str();
    }
    {
      util::ArrayFormatter formatter(80, 2);
      std::size_t accum = 0;  // used to compute cumulative sum over threshold counts
      for (auto const& e : node->cut_pts) {
        formatter << accum;
        accum += e.size();  // e.size() = number of thresholds for each feature
      }
      total_num_threshold = accum;
      array_th_begin = formatter.str();
    }
    {
      util::ArrayFormatter formatter(80, 2);
      for (auto const& e : node->cut_pts) {
        formatter << e.size();
      }
      array_th_len = formatter.str();
    }
    if (!array_threshold.empty() && !array_th_begin.empty() && !array_th_len.empty()) {
      PrependToBuffer(dest,
          fmt::format(templates::qnode_template, "total_num_threshold"_a = total_num_threshold,
              "threshold_type"_a = threshold_type),
          0);
      AppendToBuffer(dest,
          fmt::format(templates::quantize_loop_template, "num_feature"_a = num_feature_), indent);
    }
    if (!array_threshold.empty()) {
      PrependToBuffer(dest,
          fmt::format("static const {threshold_type} threshold[] = {{\n"
                      "{array_threshold}\n"
                      "}};\n",
              "array_threshold"_a = array_threshold, "threshold_type"_a = threshold_type),
          0);
    }
    if (!array_th_begin.empty()) {
      PrependToBuffer(dest,
          fmt::format("static const int th_begin[] = {{\n"
                      "{array_th_begin}\n"
                      "}};\n",
              "array_th_begin"_a = array_th_begin),
          0);
    }
    if (!array_th_len.empty()) {
      PrependToBuffer(dest,
          fmt::format("static const int th_len[] = {{\n"
                      "{array_th_len}\n"
                      "}};\n",
              "array_th_len"_a = array_th_len),
          0);
    }
    TL2CGEN_CHECK_EQ(node->children.size(), 1);
    WalkAST<ThresholdType, LeafOutputType>(node->children[0], dest, indent);
  }

  template <typename ThresholdType, typename LeafOutputType>
  void HandleCodeFolderNode(
      ast::CodeFolderNode const* node, std::string const& dest, std::size_t indent) {
    TL2CGEN_CHECK_EQ(node->children.size(), 1);
    int const node_id = node->children[0]->node_id;
    int const tree_id = node->children[0]->tree_id;

    /* render arrays needed for folding subtrees */
    std::string array_nodes, array_cat_bitmap, array_cat_begin;
    // node_treeXX_nodeXX[] : information of nodes for a particular subtree
    const std::string node_array_name = fmt::format("node_tree{}_node{}", tree_id, node_id);
    // cat_bitmap_treeXX_nodeXX[] : list of all 64-bit integer bitmaps, used to
    //                              make all categorical splits in a particular
    //                              subtree
    const std::string cat_bitmap_name = fmt::format("cat_bitmap_tree{}_node{}", tree_id, node_id);
    // cat_begin_treeXX_nodeXX[] : shows which bitmaps belong to each split.
    //                             cat_bitmap[ cat_begin[i]:cat_begin[i+1] ]
    //                             belongs to the i-th (categorical) split
    const std::string cat_begin_name = fmt::format("cat_begin_tree{}_node{}", tree_id, node_id);

    std::string output_switch_statement;
    treelite::Operator common_comp_op;
    util::RenderCodeFolderArrays<ThresholdType, LeafOutputType>(
        node, param_.quantize, false,
        "{{ {default_left}, {split_index}, {threshold}, {left_child}, {right_child} }}",
        [this](ast::OutputNode<LeafOutputType> const* node) { return RenderOutputStatement(node); },
        &array_nodes, &array_cat_bitmap, &array_cat_begin, &output_switch_statement,
        &common_comp_op);
    if (!array_nodes.empty()) {
      AppendToBuffer("header.h",
          fmt::format("extern const struct Node {node_array_name}[];\n",
              "node_array_name"_a = node_array_name),
          0);
      AppendToBuffer("arrays.c",
          fmt::format("const struct Node {node_array_name}[] = {{\n"
                      "{array_nodes}\n"
                      "}};\n",
              "node_array_name"_a = node_array_name, "array_nodes"_a = array_nodes),
          0);
    }

    if (!array_cat_bitmap.empty()) {
      AppendToBuffer("header.h",
          fmt::format("extern const uint64_t {cat_bitmap_name}[];\n",
              "cat_bitmap_name"_a = cat_bitmap_name),
          0);
      AppendToBuffer("arrays.c",
          fmt::format("const uint64_t {cat_bitmap_name}[] = {{\n"
                      "{array_cat_bitmap}\n"
                      "}};\n",
              "cat_bitmap_name"_a = cat_bitmap_name, "array_cat_bitmap"_a = array_cat_bitmap),
          0);
    }

    if (!array_cat_begin.empty()) {
      AppendToBuffer("header.h",
          fmt::format(
              "extern const size_t {cat_begin_name}[];\n", "cat_begin_name"_a = cat_begin_name),
          0);
      AppendToBuffer("arrays.c",
          fmt::format("const size_t {cat_begin_name}[] = {{\n"
                      "{array_cat_begin}\n"
                      "}};\n",
              "cat_begin_name"_a = cat_begin_name, "array_cat_begin"_a = array_cat_begin),
          0);
    }

    if (array_nodes.empty()) {
      /* folded code consists of a single leaf node */
      AppendToBuffer(dest,
          fmt::format("nid = -1;\n"
                      "{output_switch_statement}\n",
              "output_switch_statement"_a = output_switch_statement),
          indent);
    } else if (!array_cat_bitmap.empty() && !array_cat_begin.empty()) {
      AppendToBuffer(dest,
          fmt::format(templates::eval_loop_template, "node_array_name"_a = node_array_name,
              "cat_bitmap_name"_a = cat_bitmap_name, "cat_begin_name"_a = cat_begin_name,
              "data_field"_a = (param_.quantize > 0 ? "qvalue" : "fvalue"),
              "comp_op"_a = OpName(common_comp_op),
              "output_switch_statement"_a = output_switch_statement),
          indent);
    } else {
      AppendToBuffer(dest,
          fmt::format(templates::eval_loop_template_without_categorical_feature,
              "node_array_name"_a = node_array_name,
              "data_field"_a = (param_.quantize > 0 ? "qvalue" : "fvalue"),
              "comp_op"_a = OpName(common_comp_op),
              "output_switch_statement"_a = output_switch_statement),
          indent);
    }
  }

  template <typename ThresholdType>
  inline std::string ExtractNumericalCondition(
      ast::NumericalConditionNode<ThresholdType> const* node) {
    const std::string threshold_type
        = templates::TypeInfoToCTypeString(treelite::TypeToInfo<ThresholdType>());
    std::string result;
    if (node->quantized) {  // quantized threshold
      std::string lhs
          = fmt::format("data[{split_index}].qvalue", "split_index"_a = node->split_index);
      result = fmt::format("{lhs} {opname} {threshold}", "lhs"_a = lhs,
          "opname"_a = OpName(node->op), "threshold"_a = node->threshold.int_val);
    } else if (std::isinf(node->threshold.float_val)) {  // infinite threshold
      // According to IEEE 754, the result of comparison [lhs] < infinity
      // must be identical for all finite [lhs]. Same goes for operator >.
      result = (CompareWithOp(static_cast<ThresholdType>(0), node->op, node->threshold.float_val)
                    ? "1"
                    : "0");
    } else {  // finite threshold
      std::string lhs
          = fmt::format("data[{split_index}].fvalue", "split_index"_a = node->split_index);
      result = fmt::format("{lhs} {opname} ({threshold_type}){threshold}", "lhs"_a = lhs,
          "opname"_a = OpName(node->op), "threshold_type"_a = threshold_type,
          "threshold"_a = util::ToStringHighPrecision(node->threshold.float_val));
    }
    return result;
  }

  inline std::string ExtractCategoricalCondition(ast::CategoricalConditionNode const* node) {
    std::string result;
    std::vector<std::uint64_t> bitmap = util::GetCategoricalBitmap(node->matching_categories);
    TL2CGEN_CHECK_GE(bitmap.size(), 1);
    bool all_zeros = true;
    for (std::uint64_t e : bitmap) {
      all_zeros &= (e == 0);
    }
    if (all_zeros) {
      result = "0";
    } else {
      std::ostringstream oss;
      const std::string right_categories_flag = (node->categories_list_right_child ? "!" : "");
      if (node->default_left) {
        oss << fmt::format(
            "data[{split_index}].missing == -1 || {right_categories_flag}("
            "(tmp = (unsigned int)(data[{split_index}].fvalue) ), ",
            "split_index"_a = node->split_index, "right_categories_flag"_a = right_categories_flag);
      } else {
        oss << fmt::format(
            "data[{split_index}].missing != -1 && {right_categories_flag}("
            "(tmp = (unsigned int)(data[{split_index}].fvalue) ), ",
            "split_index"_a = node->split_index, "right_categories_flag"_a = right_categories_flag);
      }

      oss << fmt::format(
          "((data[{split_index}].fvalue >= 0) && "
          "(fabsf(data[{split_index}].fvalue) <= (float)(1U << FLT_MANT_DIG)) && (",
          "split_index"_a = node->split_index);
      oss << "(tmp >= 0 && tmp < 64 && (( (uint64_t)" << bitmap[0] << "U >> tmp) & 1) )";
      for (std::size_t i = 1; i < bitmap.size(); ++i) {
        oss << " || (tmp >= " << (i * 64) << " && tmp < " << ((i + 1) * 64) << " && (( (uint64_t)"
            << bitmap[i] << "U >> (tmp - " << (i * 64) << ") ) & 1) )";
      }
      oss << ")))";
      result = oss.str();
    }
    return result;
  }

  inline std::string RenderIsCategoricalArray(std::vector<bool> const& is_categorical) {
    util::ArrayFormatter formatter(80, 2);
    for (int fid = 0; fid < num_feature_; ++fid) {
      formatter << (is_categorical[fid] ? 1 : 0);
    }
    return formatter.str();
  }

  template <typename LeafOutputType>
  inline std::string RenderOutputStatement(ast::OutputNode<LeafOutputType> const* node) {
    const std::string leaf_output_type
        = templates::TypeInfoToCTypeString(treelite::TypeToInfo<LeafOutputType>());
    std::string output_statement;
    if (task_param_.num_class > 1) {
      if (node->is_vector) {
        // multi-class classification with random forest
        TL2CGEN_CHECK_EQ(node->vector.size(), static_cast<std::size_t>(task_param_.num_class))
            << "Ill-formed model: leaf vector must be of length [num_class]";
        for (std::size_t group_id = 0; group_id < task_param_.num_class; ++group_id) {
          output_statement += fmt::format("sum[{group_id}] += ({leaf_output_type}){output};\n",
              "group_id"_a = group_id,
              "output"_a = util::ToStringHighPrecision(node->vector[group_id]),
              "leaf_output_type"_a = leaf_output_type);
        }
      } else {
        // multi-class classification with gradient boosted trees
        output_statement = fmt::format("sum[{group_id}] += ({leaf_output_type}){output};\n",
            "group_id"_a = node->tree_id % task_param_.num_class,
            "output"_a = util::ToStringHighPrecision(node->scalar),
            "leaf_output_type"_a = leaf_output_type);
      }
    } else {
      output_statement = fmt::format("sum += ({leaf_output_type}){output};\n",
          "output"_a = util::ToStringHighPrecision(node->scalar),
          "leaf_output_type"_a = leaf_output_type);
    }
    return output_statement;
  }
};

ASTNativeCompiler::ASTNativeCompiler(CompilerParam const& param)
    : pimpl_(std::make_unique<ASTNativeCompilerImpl>(param)) {
  if (param.verbose > 0) {
    TL2CGEN_LOG(INFO) << "Using ASTNativeCompiler";
  }
  if (param.dump_array_as_elf > 0) {
    TL2CGEN_LOG(INFO) << "Warning: 'dump_array_as_elf' parameter is not applicable "
                         "for ASTNativeCompiler";
  }
}

ASTNativeCompiler::~ASTNativeCompiler() = default;

CompiledModel ASTNativeCompiler::Compile(treelite::Model const& model) {
  return pimpl_->Compile(model);
}

CompilerParam ASTNativeCompiler::QueryParam() const {
  return pimpl_->QueryParam();
}

}  // namespace tl2cgen::compiler::detail
