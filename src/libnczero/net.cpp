#include <nczero/log.h>
#include <nczero/net.h>

#include <ATen/Context.h>
#include <c10/core/DeviceType.h>
#include <c10/util/Exception.h>

#include <torch/script.h>

using namespace neocortex;
using namespace std;

static torch::jit::script::Module model;
static string latest_model_path;

void nn::init(bool allow_gen) {
	filesystem::path p(nn::MODEL_DIR_PATH);

	p /= nn::MODEL_LATEST_NAME;
	p /= nn::MODEL_FILENAME;

	latest_model_path = p.string();
	model = torch::jit::load(latest_model_path);

	if (torch::hasCUDA()) {
		neocortex_info("CUDA acceleration enabled\n");
		model.to(torch::kCUDA);
	}
}

std::vector<nn::output> nn::evaluate(float* inp_board, float* inp_lmm, int batch_size) {
	std::vector<torch::jit::IValue> inputs;
	std::vector<output> outputs;

	auto btensor = torch::from_blob(inp_board, {batch_size, 8, 8, SQUARE_BITS}, at::kFloat);
	auto lmmtensor = torch::from_blob(inp_lmm, {batch_size, 4096}, at::kFloat);

	if (torch::hasCUDA()) {
		btensor = btensor.to(torch::kCUDA);
		lmmtensor = lmmtensor.to(torch::kCUDA);
	}

	inputs.push_back(std::move(btensor));
	inputs.push_back(std::move(lmmtensor));

	auto output_tuple = model.forward(inputs).toTuple();
	auto output_policy = output_tuple->elements()[0].toTensor().to(torch::kCPU);
	auto output_value = output_tuple->elements()[1].toTensor().to(torch::kCPU);

	for (int i = 0; i < batch_size; ++i) {
		outputs.emplace_back();
		memcpy(outputs.back().policy, output_policy.data_ptr<float>() + (i * 4096), sizeof(float) * 4096);
		outputs.back().value = *(output_value.data_ptr<float>() + i);
	}

	return outputs;
}