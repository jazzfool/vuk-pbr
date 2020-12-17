#pragma once

#include "Types.hpp"

#include <vuk/Pipeline.hpp>
#include <unordered_map>
#include <string_view>

namespace vuk {
class Context;
}

/*
	Loads shaders as normal from embedded resources when built in release mode, but reloads shaders on the fly from disk when built in debug mode.
*/

class PipelineStore {
  public:
	PipelineStore();
	PipelineStore(vuk::Context& ctxt);

	void add(std::string_view name, std::string_view vert, std::string_view frag, vuk::PipelineBaseCreateInfo base = {});

	void update();

  private:
	void load(std::string_view name);

	struct Pipe {
		std::string vert;
		std::string frag;
		vuk::PipelineBaseCreateInfo pipe;
	};

	u32 m_counter;
	std::unordered_map<std::string, Pipe> m_pipes;
	vuk::Context* m_ctxt;
};
