#include <c10/util/Exception.h>
#include <nczero/net.h>

#include <filesystem>

using namespace neocortex::nn;

Network::Network(std::string path) {
	module = torch::jit::load(path);
}

std::vector<Network::Output> Network::evaluate(std::vector<float>& inp_board, std::vector<float>& inp_lmm, int batch_size) {
	std::vector<torch::jit::IValue> inputs;
	std::vector<Output> outputs;

	inputs.push_back(std::move(torch::from_blob(&inp_board[0], {batch_size, 8, 8, 85}, at::kFloat)));
	inputs.push_back(std::move(torch::from_blob(&inp_lmm[0], {batch_size, 4096}, at::kFloat)));

	auto output_tuple = module.forward(inputs).toTuple();
	auto output_policy = output_tuple->elements()[0].toTensor();
	auto output_value = output_tuple->elements()[1].toTensor();

	for (int i = 0; i < batch_size; ++i) {
		outputs.emplace_back();
		memcpy(outputs.back().policy, output_policy.data_ptr<float>() + (i * 4096), sizeof(float) * 4096);
		outputs.back().value = *(output_value.data_ptr<float>() + i);
	}

	return outputs;
}
