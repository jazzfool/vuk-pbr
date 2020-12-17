#include "PipelineStore.hpp"

#include "Resource.hpp"

#include <vuk/Context.hpp>
#include <fstream>
#include <streambuf>

PipelineStore::PipelineStore() : m_counter{0}, m_ctxt{nullptr} {
}

PipelineStore::PipelineStore(vuk::Context& ctxt) : m_counter{0}, m_ctxt{&ctxt} {
}

void PipelineStore::add(std::string_view name, std::string_view vert, std::string_view frag, vuk::PipelineBaseCreateInfo base) {
	m_pipes[std::string{name}] = Pipe{std::string{vert}, std::string{frag}, base};
	load(name);
}

void PipelineStore::update() {
#ifndef NDEBUG
	m_counter++;

	if (m_counter > 1000) {
		m_counter = 0;

		for (const auto& [k, _] : m_pipes) {
			load(k);
		}
	}
#endif
}

void PipelineStore::load(std::string_view name) {
	const Pipe& p = m_pipes.at(std::string{name});
	vuk::PipelineBaseCreateInfo pipe = p.pipe;
#ifndef NDEBUG
	std::ifstream vertf{std::string{PROJECT_ABSOLUTE_PATH} + std::string{"/Resources/Shaders/"} + p.vert};
	std::string vert{(std::istreambuf_iterator<char>(vertf)), std::istreambuf_iterator<char>()};
	vertf.close();

	std::ifstream fragf{std::string{PROJECT_ABSOLUTE_PATH} + std::string{"/Resources/Shaders/"} + p.frag};
	std::string frag{(std::istreambuf_iterator<char>(fragf)), std::istreambuf_iterator<char>()};
	fragf.close();

	pipe.add_shader(vert, p.vert);
	pipe.add_shader(frag, p.frag);
#else
	pipe.add_shader(get_resource_string(std::string{"Resources/Shaders/"} + p.vert), p.vert);
	pipe.add_shader(get_resource_string(std::string{"Resources/Shaders/"} + p.frag), p.frag);
#endif
	m_ctxt->create_named_pipeline(name.data(), pipe);
}
