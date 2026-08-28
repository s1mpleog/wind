#include "scene.hpp"
#include "render_object.hpp"
#include "resources/builtin.hpp"
#include "resources/resource_manager.hpp"

namespace wind::scene {
// auto Scene::add_render_objects(resources::ModelHandle           model_handle,
//                                vulkan::graphics::PipelineHandle pipeline_handle,
//                                Transform /*unused*/) WIND_NOEXCEPT -> void
// {
//   auto render_object = RenderObject{
//       .model_handle    = model_handle,
//       .pipeline_handle = pipeline_handle,

//   };

//   m_objects.push_back(render_object);
// }

auto Scene::add_render_objects(builtin::BuiltAssets assets, bool is_model, Transform transform) WIND_NOEXCEPT -> void
{
  auto render_object = RenderObject{.model_handle    = is_model ? assets.models : resources::ModelHandle{},
                                    .pipeline_handle = assets.pipelines,
                                    .is_model_type   = is_model,
                                    .buffer_asset    = is_model ? builtin::BufferAssets{} : assets.buffer_assets};

  m_objects.push_back(render_object);
}

auto Scene::get() WIND_NOEXCEPT -> std::vector<RenderObject>
{
  return m_objects;
}

};  // namespace wind::scene
