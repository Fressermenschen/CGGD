#include "renderer.h"

#include "utils/error_handler.h"

#ifdef RASTERIZATION
#include "renderer/rasterizer/rasterizer_renderer.h"
#endif

#ifdef RAYTRACING
#include "renderer/raytracer/raytracer_renderer.h"
#endif

#ifdef DX12
#include "renderer/dx12/dx12_renderer.h"
#endif


using namespace cg::renderer;

void cg::renderer::renderer::set_settings(std::shared_ptr<cg::settings> in_settings)
{
	settings = in_settings;
}

unsigned cg::renderer::renderer::get_height()
{
	return settings->height;
}

unsigned cg::renderer::renderer::get_width()
{
	return settings->width;
}


std::shared_ptr<renderer> cg::renderer::make_renderer(std::shared_ptr<cg::settings> settings)
{
#ifdef RASTERIZATION
	auto renderer = std::make_shared<cg::renderer::rasterization_renderer>();
	renderer->set_settings(settings);
	return renderer;
#endif
#ifdef RAYTRACING
	auto renderer = std::make_shared<cg::renderer::ray_tracing_renderer>();
	renderer->set_settings(settings);
	return renderer;
#endif
#ifdef DX12
	auto renderer = std::make_shared<cg::renderer::dx12_renderer>();
	renderer->set_settings(settings);
	return renderer;
#endif

	THROW_ERROR("Type of renderer is not selected");
}

void cg::renderer::renderer::move_forward(float delta)
{
	DirectX::XMVECTOR forward = DirectX::XMVectorScale(camera->get_direction(), delta);
	camera->set_position(DirectX::XMVectorAdd(camera->get_position(), forward));
}

void cg::renderer::renderer::move_backward(float delta)
{
	DirectX::XMVECTOR forward = DirectX::XMVectorScale(camera->get_direction(), delta);
	camera->set_position(DirectX::XMVectorSubtract(camera->get_position(), forward));
}

void cg::renderer::renderer::move_left(float delta)
{
	DirectX::XMVECTOR right = DirectX::XMVectorScale(camera->get_right(), delta);
	camera->set_position(DirectX::XMVectorSubtract(camera->get_position(), right));
}

void cg::renderer::renderer::move_right(float delta)
{
	DirectX::XMVECTOR right = DirectX::XMVectorScale(camera->get_right(), delta);
	camera->set_position(DirectX::XMVectorAdd(camera->get_position(), right));
}

void cg::renderer::renderer::move_up(float delta)
{
	DirectX::XMVECTOR up = DirectX::XMVectorScale(camera->get_up(), delta);
	camera->set_position(DirectX::XMVectorAdd(camera->get_position(), up));
}

void cg::renderer::renderer::move_down(float delta)
{
	DirectX::XMVECTOR up = DirectX::XMVectorScale(camera->get_right(), delta);
	camera->set_position(DirectX::XMVectorSubtract(camera->get_position(), up));
}

void cg::renderer::renderer::move_yaw(float delta)
{
	camera->set_theta(camera->get_theta() + delta);
}

void cg::renderer::renderer::move_pitch(float delta)
{
	camera->set_phi(camera->get_phi() + delta);
}