#include "SSAO.hpp"

#include "../Context.hpp"
#include "../Resource.hpp"
#include "../Mesh.hpp"
#include "../Scene.hpp"
#include "../GfxUtil.hpp"
#include "../Renderer.hpp"

#include <vuk/CommandBuffer.hpp>
#include <random>

void SSAODepthPass::setup(Context& ctxt) {
	vuk::PipelineBaseCreateInfo ssao;
	ssao.add_shader(get_resource_string("Resources/Shaders/ssao.vert"), "ssao.vert");
	ssao.add_shader(get_resource_string("Resources/Shaders/ssao.frag"), "ssao.frag");
	ctxt.vuk_context->create_named_pipeline("ssao", ssao);

	vuk::PipelineBaseCreateInfo blur;
	blur.add_shader(get_resource_string("Resources/Shaders/ssao.vert"), "ssao.vert");
	blur.add_shader(get_resource_string("Resources/Shaders/ssao_blur.frag"), "ssao_blur.frag");
	ctxt.vuk_context->create_named_pipeline("ssao_blur", blur);
}

void SSAODepthPass::init(vuk::PerThreadContext& ptc) {
	m_random_normal = gfx_util::load_texture("Resources/Textures/random_normal.jpg", ptc, false);

	// generate kernel

	std::uniform_real_distribution<f32> random_floats{0.f, 1.f};
	std::default_random_engine generator;
	for (u16 i = 0; i < KERNEL_SIZE; ++i) {
		glm::vec3 sample{random_floats(generator) * 2.f - 1.f, random_floats(generator) * 2.f - 1.f, random_floats(generator)};
		sample = glm::normalize(sample);
		sample *= random_floats(generator);
		f32 scale = static_cast<f32>(i) / static_cast<f32>(KERNEL_SIZE);

		scale = std::lerp(0.1f, 1.f, scale * scale);
		sample *= scale;
		m_kernel[i] = sample;
	}

	// TODO(jazzfool): generate noise texture instead of loading it
}

void SSAODepthPass::build(vuk::PerThreadContext& ptc, vuk::RenderGraph& rg, const RenderInfo& info) {
	struct Uniforms {
		std::array<glm::vec4, KERNEL_SIZE> samples;
		glm::mat4 projection;
	} uniforms;

	for (u16 i = 0; i < KERNEL_SIZE; ++i) {
		uniforms.samples[i] = glm::vec4{m_kernel[i], 1.f};
	}

	uniforms.projection = info.cam_proj.matrix();

	auto [bubo, stub] = ptc.create_scratch_buffer(vuk::MemoryUsage::eCPUtoGPU, vuk::BufferUsageFlagBits::eUniformBuffer, std::span{&uniforms, 1});
	auto ubo = bubo;

	ptc.wait_all_transfers();

	auto ssao_pass =
		vuk::Pass{.resources = {"ssao"_image(vuk::eColorWrite), "g_position"_image(vuk::eFragmentSampled), "g_normal"_image(vuk::eFragmentSampled)},
				  .execute = [this, ubo](vuk::CommandBuffer& cbuf) {
					  const auto sci = vuk::SamplerCreateInfo{
						  .addressModeU = vuk::SamplerAddressMode::eClampToEdge,
						  .addressModeV = vuk::SamplerAddressMode::eClampToEdge,
						  .addressModeW = vuk::SamplerAddressMode::eClampToEdge,
					  };

					  cbuf.bind_graphics_pipeline("ssao")
						  .bind_sampled_image(0, 0, "g_position", sci)
						  .bind_sampled_image(0, 1, "g_normal", sci)
						  .bind_sampled_image(0, 2, m_random_normal,
											  {
												  .addressModeU = vuk::SamplerAddressMode::eRepeat,
												  .addressModeV = vuk::SamplerAddressMode::eRepeat,
												  .addressModeW = vuk::SamplerAddressMode::eRepeat,
											  })
						  .bind_uniform_buffer(0, 3, ubo)
						  .push_constants(vuk::ShaderStageFlagBits::eFragment, 0, glm::vec2{width, height})
						  .draw(3, 1, 0, 0);
				  }};

	auto blur_pass = vuk::Pass{
		.resources = {"ssao_blurred"_image(vuk::eColorWrite), "ssao"_image(vuk::eFragmentSampled)}, .execute = [](vuk::CommandBuffer& cbuf) {
			const auto sci = vuk::SamplerCreateInfo{
				.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
				.addressModeV = vuk::SamplerAddressMode::eClampToEdge,
				.addressModeW = vuk::SamplerAddressMode::eClampToEdge,
			};

			cbuf.set_viewport(0, vuk::Rect2D::framebuffer()).bind_graphics_pipeline("ssao_blur").bind_sampled_image(0, 0, "ssao", sci).draw(3, 1, 0, 0);
		}};

	rg.add_pass(ssao_pass);
	rg.add_pass(blur_pass);

	rg.attach_managed("ssao", vuk::Format::eR16Sfloat, vuk::Dimension2D::absolute(width, height), vuk::Samples::e1, vuk::ClearColor{0.f, 0.f, 0.f, 1.f});
	rg.attach_managed("ssao_blurred", vuk::Format::eR16Sfloat, vuk::Dimension2D::absolute(width, height), vuk::Samples::e1,
					  vuk::ClearColor{0.f, 0.f, 0.f, 1.f});
}
