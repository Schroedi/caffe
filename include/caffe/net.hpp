#ifndef CAFFE_NET_HPP_
#define CAFFE_NET_HPP_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "caffe/blob.hpp"
#include "caffe/common.hpp"
#include "caffe/layer.hpp"
#include "caffe/proto/caffe.pb.h"

namespace caffe {

template <typename Dtype>
class Net {
 public:
  explicit Net(const NetParameter& param);
  explicit Net(const string& param_file);
  virtual ~Net() {}

  // Initialize a network with the network parameter.
  void Init(const NetParameter& param);

  // Run forward with the input blobs already fed separately. You can get the
  // input blobs using input_blobs().
  const vector<Blob<Dtype>*>& ForwardPrefilled(Dtype* loss = NULL);

  // The From and To variants of Forward and Backward operate on the
  // (topological) ordering by which the net is specified. For general DAG
  // networks, note that (1) computing from one layer to another might entail
  // extra computation on unrelated branches, and (2) computation starting in
  // the middle may be incorrect if all of the layers of a fan-in are not
  // included.
  Dtype ForwardFromTo(int start, int end);
  Dtype ForwardFrom(int start);
  Dtype ForwardTo(int end);
  // Run forward using a set of bottom blobs, and return the result.
  const vector<Blob<Dtype>*>& Forward(const vector<Blob<Dtype>* > & bottom,
      Dtype* loss = NULL);
  // Run forward using a serialized BlobProtoVector and return the result
  // as a serialized BlobProtoVector
  string Forward(const string& input_blob_protos, Dtype* loss = NULL);

  // The network backward should take no input and output, since it solely
  // computes the gradient w.r.t the parameters, and the data has already
  // been provided during the forward pass.
  void Backward();
  void BackwardFromTo(int start, int end);
  void BackwardFrom(int start);
  void BackwardTo(int end);

  Dtype ForwardBackward(const vector<Blob<Dtype>* > & bottom) {
    Dtype loss;
    Forward(bottom, &loss);
    Backward();
    return loss;
  }

  // Updates the network weights based on the diff values computed.
  void Update();

  // For an already initialized net, ShareTrainedLayersWith() implicitly copies
  // (i.e., using no additional memory) the already trained layers from another
  // Net.
  void ShareTrainedLayersWith(Net* other);
  // For an already initialized net, CopyTrainedLayersFrom() copies the already
  // trained layers from another net parameter instance.
  void CopyTrainedLayersFrom(const NetParameter& param);
  void CopyTrainedLayersFrom(const string trained_filename);
  // Writes the net to a proto.
  void ToProto(NetParameter* param, bool write_diff = false);

  // returns the network name.
  inline const string& name() { return name_; }
  // returns the layer names
  inline const vector<string>& layer_names() { return layer_names_; }
  // returns the blob names
  inline const vector<string>& blob_names() { return blob_names_; }
  // returns the blobs
  inline const vector<shared_ptr<Blob<Dtype> > >& blobs() { return blobs_; }
  // returns the layers
  inline const vector<shared_ptr<Layer<Dtype> > >& layers() { return layers_; }
  // returns the bottom and top vecs for each layer - usually you won't need
  // this unless you do per-layer checks such as gradients.
  inline vector<vector<Blob<Dtype>*> >& bottom_vecs() { return bottom_vecs_; }
  inline vector<vector<Blob<Dtype>*> >& top_vecs() { return top_vecs_; }
  inline vector<vector<bool> >& bottom_need_backward() {
    return bottom_need_backward_;
  }
  inline vector<Dtype>& blob_loss_weights() {
    return blob_loss_weights_;
  }
  // returns the parameters
  inline vector<shared_ptr<Blob<Dtype> > >& params() { return params_; }
  // returns the parameter learning rate multipliers
  inline vector<float>& params_lr() { return params_lr_; }
  inline vector<float>& params_weight_decay() { return params_weight_decay_; }
  const map<string, int>& param_names_index() { return param_names_index_; }
  // Input and output blob numbers
  inline int num_inputs() { return net_input_blobs_.size(); }
  inline int num_outputs() { return net_output_blobs_.size(); }
  inline vector<Blob<Dtype>*>& input_blobs() { return net_input_blobs_; }
  inline vector<Blob<Dtype>*>& output_blobs() { return net_output_blobs_; }
  inline vector<int>& input_blob_indices() { return net_input_blob_indices_; }
  inline vector<int>& output_blob_indices() { return net_output_blob_indices_; }
  // has_blob and blob_by_name are inspired by
  // https://github.com/kencoken/caffe/commit/f36e71569455c9fbb4bf8a63c2d53224e32a4e7b
  // Access intermediary computation layers, testing with centre image only
  bool has_blob(const string& blob_name);
  const shared_ptr<Blob<Dtype> > blob_by_name(const string& blob_name);
  bool has_layer(const string& layer_name);
  const shared_ptr<Layer<Dtype> > layer_by_name(const string& layer_name);

  void set_debug_info(const bool value) { debug_info_ = value; }

  // Helpers for Init.
  // Remove layers that the user specified should be excluded given the current
  // phase, level, and stage.
  static void FilterNet(const NetParameter& param,
      NetParameter* param_filtered);
  static bool StateMeetsRule(const NetState& state, const NetStateRule& rule,
      const string& layer_name);

 protected:
  // Helpers for Init.
  // Append a new input or top blob to the net.
  void AppendTop(const NetParameter& param, const int layer_id,
                 const int top_id, set<string>* available_blobs,
                 map<string, int>* blob_name_to_idx);
  // Append a new bottom blob to the net.
  int AppendBottom(const NetParameter& param, const int layer_id,
                   const int bottom_id, set<string>* available_blobs,
                   map<string, int>* blob_name_to_idx);
  void AppendParam(const NetParameter& param, const int layer_id,
                   const int param_id);

  // Helpers for displaying debug info.
  void ForwardDebugInfo(const int layer_id);
  void BackwardDebugInfo(const int layer_id);
  void UpdateDebugInfo(const int param_id);

  // Function to get misc parameters, e.g. the learning rate multiplier and
  // weight decay.
  void GetLearningRateAndWeightDecay();

  // Individual layers in the net
  vector<shared_ptr<Layer<Dtype> > > layers_;
  vector<string> layer_names_;
  map<string, int> layer_names_index_;
  vector<bool> layer_need_backward_;
  // blobs stores the blobs that store intermediate results between the
  // layers.
  vector<shared_ptr<Blob<Dtype> > > blobs_;
  vector<string> blob_names_;
  map<string, int> blob_names_index_;
  vector<bool> blob_need_backward_;
  // bottom_vecs stores the vectors containing the input for each layer.
  // They don't actually host the blobs (blobs_ does), so we simply store
  // pointers.
  vector<vector<Blob<Dtype>*> > bottom_vecs_;
  vector<vector<int> > bottom_id_vecs_;
  vector<vector<bool> > bottom_need_backward_;
  // top_vecs stores the vectors containing the output for each layer
  vector<vector<Blob<Dtype>*> > top_vecs_;
  vector<vector<int> > top_id_vecs_;
  // Vector of weight in the loss (or objective) function of each net blob,
  // indexed by blob_id.
  vector<Dtype> blob_loss_weights_;
  vector<int> param_owners_;
  vector<string> param_display_names_;
  vector<pair<int, int> > param_layer_indices_;
  map<string, int> param_names_index_;
  // blob indices for the input and the output of the net
  vector<int> net_input_blob_indices_;
  vector<int> net_output_blob_indices_;
  vector<Blob<Dtype>*> net_input_blobs_;
  vector<Blob<Dtype>*> net_output_blobs_;
  string name_;
  // The parameters in the network.
  vector<shared_ptr<Blob<Dtype> > > params_;
  // the learning rate multipliers
  vector<float> params_lr_;
  // the weight decay multipliers
  vector<float> params_weight_decay_;
  // The bytes of memory used by this net
  size_t memory_used_;
  // Whether to compute and display debug info for the net.
  bool debug_info_;

  DISABLE_COPY_AND_ASSIGN(Net);
};


}  // namespace caffe

#endif  // CAFFE_NET_HPP_
