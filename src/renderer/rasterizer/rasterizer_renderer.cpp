#include "rasterizer_renderer.h"

#include "utils/resource_utils.h"

#include <DirectXMath.h>


void cg::renderer::rasterization_renderer::init()
{
	//THROW_ERROR("Not implemented yet");

	// Create RT an DB
	render_target = std::make_shared<resource<unsigned_color>>(get_width(), get_height());
	depth_buffer = std::make_shared<resource<float>>(get_width(), get_height());

	// Create rasterizer instance
	rasterizer = std::make_shared<cg::renderer::rasterizer<vertex, unsigned_color>>();
	rasterizer->set_render_target(render_target, depth_buffer);
	rasterizer->set_viewport(get_width(), get_height());

	// Setup camera settings
	const DirectX::XMFLOAT3 camera_position{
		settings->camera_position[0],
		settings->camera_position[1],
		settings->camera_position[2]
	};

	camera = std::make_shared<cg::world::camera>();
	camera->set_position(DirectX::XMLoadFloat3(&camera_position));
	camera->set_angle_of_view(settings->camera_angle_of_view);
	camera->set_height(static_cast<float>(settings->height));
	camera->set_width(static_cast<float>(settings->width));
	camera->set_theta(settings->camera_phi);
	camera->set_phi(settings->camera_theta);
	camera->set_z_near(settings->camera_z_near);
	camera->set_z_far(settings->camera_z_far);

	// Load model from file
	model = std::make_shared<cg::world::model>();
	model->load_obj(settings->model_path);

	// Add vertex shader
	rasterizer->vertex_shader = [this](vertex vertex_data) {
		// Collect transformation matrices
		const DirectX::XMMATRIX world = (model->get_world_matrix());
		const DirectX::XMMATRIX view = (camera->get_view_matrix());
		const DirectX::XMMATRIX projection = (camera->get_projection_matrix());

		DirectX::XMVECTOR address = DirectX::XMLoadFloat3(&vertex_data.position);

		address = DirectX::XMVector3Project(address, 0.0f, 0.0f,
											static_cast<float>(settings->width),
											static_cast<float>(settings->height),
											settings->camera_z_near,
											settings->camera_z_far,
											projection, view, world);

		DirectX::XMStoreFloat3(&vertex_data.position, address);
		return vertex_data;
	};

	rasterizer->pixel_shader = [this](vertex vertex_data, const float b, const float z) {
		const float distance = 0.25f + 0.75f * 5000 * z;
		const float intensity = (1 - b);
		// Pixel shader renders pixels according to its depth and barycentric distance
		// from vertices. This way, vertices have black color and face centers have white.
		return color::from_float3(float3{intensity, intensity, intensity});
	};
}

void cg::renderer::rasterization_renderer::destroy() {}

void cg::renderer::rasterization_renderer::update() {}

void cg::renderer::rasterization_renderer::render()
{
	//THROW_ERROR("Not implemented yet");
	rasterizer->clear_render_target(FLT_MAX);

	auto &vertex_buffers = model->get_vertex_buffers();
	auto &index_buffers = model->get_index_buffers();

	const size_t num_shapes = vertex_buffers.size();

	// Render every shape
	for (size_t i = 0; i != num_shapes; ++i) {
		rasterizer->set_vertex_buffer(vertex_buffers[i]);
		rasterizer->set_index_buffer(index_buffers[i]);

		rasterizer->draw(index_buffers[i]->get_number_of_elements());
	}

	// Save to file and display
	utils::save_resource(*render_target, settings->result_path);
}