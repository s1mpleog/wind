#include "application.hpp"
#include "platform/window.hpp"
#include "types.hpp"
#include <cstdlib>
#include <spdlog/spdlog.h>

/*
 initial architecture

Application will own AppContext which will store VkContext, AudioContext, etc
plain POD struct with useful accessors and namespace functions

e.g.

namespace wind::vulkan {

struct Context {
...
}

auto create_instance() -> void;

auto build() {
create_instance();
...
}

}

Result<T, AppError> for all vulkan functions and anything that can failed (exceptions is disabled)

ownership design

unique_ptr<T> -> for ownership the unique_ptr is the owner of the resource so make sure borrows and lifetimes
don't outlives owner

const T* -> nullable look at read only view to resource
T* -> nullable read/write view to resource

const &T -> non null look at read only view to resource
T& -> non null read/write view to resource

VulkanContext is responsible to manage ownership of all vulkan related resources

AppContext will own sub-systems that needs to be passed around and all


Directory structure

all vulkan related code inside vulkan/
all window and presentation related code inside platform/
all rendering and drawing related code inside renderer/
optional math related code inside math/
utilities functions inside utils/


types related code inside types.h e.g. for vulkan it will be vulkan/types.h for platform it will be platform/types.h
*/
auto main() -> i32
{
  constexpr u16 WINDOW_WIDTH  = 800;
  constexpr u16 WINDOW_HEIGHT = 600;

  auto window_cfg = wind::platform::WindowConfiguration{.name = "Wind", .width = WINDOW_WIDTH, .height = WINDOW_HEIGHT};

  auto app = wind::app::init(std::move(window_cfg));

  if(!app)
  {
    spdlog::error("{}", app.error().to_string());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
