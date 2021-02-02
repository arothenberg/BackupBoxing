// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#include <librealsense2/rs.hpp>
#include "../example.hpp"
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include <imgui_internal.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>

#include <librealsense2/rs.hpp>
#include "../example.hpp"
#include <imgui.h>
#include "imgui_impl_glfw.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>

//*** We need this library for easy image loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//*** This code allows us to rotate texture in ImGui. Taken from https://github.com/ocornut/imgui/issues/1982
#include <math.h>




void get_filter_config(float& x_widthL,float& x_widthR, float& y_height,float& clipping_distance);
void render_slider(rect location, float& clipping_dist, const char* name, float vmax);

void draw_center_line(int _x, int _y, uint8_t* rgbatab, int height, int width);
void draw_circle(int top_heightx, int top_heighty, uint8_t* rgbatab, int width);
void render_slider(rect location, float& clipping_dist, const char* name, float vmax);
void remove_background(const rs2::depth_frame& depth_frame, uint8_t* rgbatab,
	float depth_scale, int x_widthL, int x_widthR, int y_height,
	float clipping_distance, std::map<std::string, float>& measures);
float get_depth_scale(rs2::device dev);
rs2_stream find_stream_to_align(const std::vector<rs2::stream_profile>& streams);
bool profile_changed(const std::vector<rs2::stream_profile>& current, const std::vector<rs2::stream_profile>& prev);
const std::string r_weight = "r_weight";
const std::string l_weight = "l_weight";
const std::string weight_ratio = "weight_ratio";
const std::string closest_distance = "closest_distance";
const std::string head_distance = "head_distance";


static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs)
{
	return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}
static inline ImVec2 ImRotate(const ImVec2& v, float cos_a, float sin_a)
{
	return ImVec2(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a);
}
void ImageRotated(ImTextureID tex_id, ImVec2 center, ImVec2 size, float angle)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();

	float cos_a = cosf(angle);
	float sin_a = sinf(angle);
	ImVec2 pos[4] =
	{
		center + ImRotate(ImVec2(-size.x * 0.5f, -size.y * 0.5f), cos_a, sin_a),
		center + ImRotate(ImVec2(+size.x * 0.5f, -size.y * 0.5f), cos_a, sin_a),
		center + ImRotate(ImVec2(+size.x * 0.5f, +size.y * 0.5f), cos_a, sin_a),
		center + ImRotate(ImVec2(-size.x * 0.5f, +size.y * 0.5f), cos_a, sin_a)
	};
	ImVec2 uvs[4] =
	{
		ImVec2(0.0f, 0.0f),
		ImVec2(1.0f, 0.0f),
		ImVec2(1.0f, 1.0f),
		ImVec2(0.0f, 1.0f)
	};

	window->DrawList->AddImageQuad(tex_id, pos[0], pos[1], pos[2], pos[3], uvs[0], uvs[1], uvs[2], uvs[3], IM_COL32_WHITE);
}

//*** Here we store textures parameters
int arrow_width = 0;
int arrow_height = 0;
unsigned char* arrow_data;
GLuint arrow_texture;

int circle_width = 0;
int circle_height = 0;
unsigned char* circle_data;
GLuint circle_texture;


int main(int argc, char * argv[]) try
{
//asdfasdf
/////wewerwrwerewrwerwerw



	float x_widthL = 0.0;
	float x_widthR = 0.0;
	float y_height = 0.0;
	float clipping_distance = 0.0;

	std::vector < std::string > needed_measures;
	needed_measures.push_back(head_distance);
	needed_measures.push_back(closest_distance);
	needed_measures.push_back(weight_ratio);
	needed_measures.push_back(r_weight);
	needed_measures.push_back(l_weight);
	std::map<std::string, float> measures;


	measures[r_weight] = 0.0;
	measures[l_weight] = 0.0;
	measures[closest_distance] = 0.0;
	measures[head_distance] = 0.0;

    get_filter_config(x_widthL,x_widthR,y_height,clipping_distance);


// Obtain a list of devices currently present on the system
rs2::context ctx;
auto devices = ctx.query_devices();
size_t device_count = devices.size();
if (!device_count)
{
    std::cout <<"No device detected. Is it plugged in?\n";
    return EXIT_SUCCESS;
}

    window app(1280, 720, "RealSense Align (Advanced) Example"); // Simple window handling
    ImGui_ImplGlfw_Init(app, false);      // ImGui library intializition
    rs2::colorizer c;                     // Helper to colorize depth images
    texture renderer;                     // Helper for renderig images

rs2::config cfg;
cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 90);


    // Create a pipeline to easily configure and start the camera
    rs2::pipeline pipe;
    rs2::pipeline_profile profile = pipe.start(cfg);

    float depth_scale = get_depth_scale(profile.get_device());
        //*** Load arrow texture
    arrow_data = stbi_load("arrow.png", &arrow_width, &arrow_height, NULL, 4);
	glGenTextures(1, &arrow_texture);
	glBindTexture(GL_TEXTURE_2D, arrow_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, arrow_width, arrow_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, arrow_data);
	stbi_image_free(arrow_data);

	//*** Load circle texture
	circle_data = stbi_load("circle.png", &circle_width, &circle_height, NULL, 4);
	glGenTextures(1, &circle_texture);
	glBindTexture(GL_TEXTURE_2D, circle_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, circle_width, circle_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, circle_data);
	stbi_image_free(circle_data);

    
    while (app) // Application still alive?
    {
        // Using the align object, we block the application until a frameset is available
        rs2::frameset frameset = pipe.wait_for_frames();

              if (profile_changed(pipe.get_active_profile().get_streams(), profile.get_streams()))
        {
            profile = pipe.get_active_profile();
            depth_scale = get_depth_scale(profile.get_device());
        }
        rs2::depth_frame aligned_depth_frame = frameset.get_depth_frame();//  processed.get_depth_frame();

        //If one of them is unavailable, continue iteration
        if (!aligned_depth_frame)// || !other_frame)
        {
            continue;
        }
         		uint8_t *rgbatab =  (uint8_t *) malloc(4*640*480 * sizeof(uint8_t)); 
            assert(rgbatab);


		remove_background(aligned_depth_frame, rgbatab, depth_scale, x_widthL, x_widthR, y_height,
			clipping_distance, measures);

	if (measures[r_weight] > 0)
		{
			measures[weight_ratio] = measures[l_weight] / measures[r_weight];
		}





        // Taking dimensions of the window for rendering purposes
        float w = static_cast<float>(app.width());
        float h = static_cast<float>(app.height());
        auto xx = aligned_depth_frame.get_data();
           // The example also renders the depth frame, as a picture-in-picture
        // Calculating the position to place the depth frame in the window
        rect pip_stream{ 0, 0, w, h };
        
        pip_stream = pip_stream.adjust_ratio({ static_cast<float>(aligned_depth_frame.get_width()),static_cast<float>(aligned_depth_frame.get_height()) });
        pip_stream.x = 0;// altered_other_frame_rect.x + altered_other_frame_rect.w - pip_stream.w - (std::max(w, h) / 25);
        pip_stream.y = 0;// altered_other_frame_rect.y + (std::max(w, h) / 25);

        // Render depth (as picture in pipcture)
        renderer.upload_aaron(rgbatab);
       renderer.show(pip_stream);

	ImGui_ImplGlfw_NewFrame(1);
		if (ImGui::Button("save", { 30, 30 }))
		{
			std::ofstream myfile;
			myfile.open("config_frame.txt");
			myfile << (int)x_widthL << "," << (int)x_widthR << "," << (int)y_height << "," << (int)clipping_distance;

			myfile.close();
		}


	for (decltype(needed_measures.size()) i = 0; i <= needed_measures.size() - 1; i++)
		{
			char l_val[40];// = needed_measures[i] + ":"; //   "left weight: ";
			std::string use_str = needed_measures[i] + ": ";
			std::strcpy(l_val, use_str.c_str());
			char l_m_val[10];
			//sprintf(l_m_val, "%.2f", measures[l_weight]);
			sprintf(l_m_val, "%.2f", measures[needed_measures[i].c_str()]);
			std::strcat(l_val, l_m_val);// , sprintf(cVal, "%.2f", 36.2343);// measures[l_weight]));
			ImGui::Text(l_val);
		}


	//	ImGui::SetCursorPos({ app.width() / 2 - 100, 3 * app.height() / 5 + 110 });

        // Using ImGui library to provide a slide controller to select the depth clipping distance
        render_slider({ 1000.f, 10, w, h/3 }, x_widthL, "left", 700);
        render_slider({ 1100.f, 10, w/2, h/3 }, y_height, "top",500);
        render_slider({ 1000.f, 300, w/2, h/3 }, x_widthR, "right",700);
        render_slider({ 1100.f, 300, w/2, h /3}, clipping_distance, "distance", 2000);



	static const int flags = ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove;

    auto& style = ImGui::GetStyle();
        style.Colors[ImGuiCol_WindowBg] = ImColor(0, 0, 0, 0);

        ImGui::SetNextWindowPos({200, 200});
		ImGui::Begin("Arrow&Circle", nullptr, flags);
        //*** Draw the circle
        ImGui::Image((void*)(intptr_t)circle_texture, ImVec2(circle_width, circle_height));

        //*** Setup the rotation angle value. It is static here (just for example), so each drawing cycle we can change the angle
		static float angle = 0.0f;
		angle += 0.02f;

        //*** Draw the arrow rotated by the current angle value. I used the manually entered rotation center position
        //as the arrow is not drawn very neatly, not in the center of the texture.
        //When the image is well centered, you can snap the center of rotation to the window coordinates. 
		ImageRotated((void*)(intptr_t)arrow_texture, ImVec2(274, 272), ImVec2(128.0f, 128.0f), angle);
		


		ImGui::End();


        ImGui::Render();

    }
    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}

float get_depth_scale(rs2::device dev)
{
    // Go over the device's sensors
    for (rs2::sensor& sensor : dev.query_sensors())
    {
        // Check if the sensor if a depth sensor
        if (rs2::depth_sensor dpt = sensor.as<rs2::depth_sensor>())
        {
            return dpt.get_depth_scale();
        }
    }
    throw std::runtime_error("Device does not have a depth sensor");
}

void render_slider(rect location, float& clipping_dist, const char *name, float vmax  )
{
    // Some trickery to display the control nicely
    static const int flags = ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove;
    const int pixels_to_buttom_of_stream_text = 25;
    const float slider_window_width = 30;

    ImGui::SetNextWindowPos({ location.x, location.y + pixels_to_buttom_of_stream_text });
    ImGui::SetNextWindowSize({ slider_window_width + 20, location.h - (pixels_to_buttom_of_stream_text * 2) });

    //Render the vertical slider
    ImGui::Begin(name, nullptr, flags);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor(215.f / 255, 215.0f / 255, 215.0f / 255));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImColor(215.f / 255, 215.0f / 255, 215.0f / 255));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImColor(215.f / 255, 215.0f / 255, 215.0f / 255));
    auto slider_size = ImVec2(slider_window_width / 2, location.h - (pixels_to_buttom_of_stream_text * 2) - 20);


        ImGui::VSliderFloat("", slider_size, &clipping_dist, 0.0f, vmax, "", 1.0f, true);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Depth Clipping Distance: %.3f", clipping_dist);
    ImGui::PopStyleColor(3);

    //Display bars next to slider
    float bars_dist = (slider_size.y / 6.0f);
    for (int i = 0; i <= 6; i++)
    {
        ImGui::SetCursorPos({ slider_size.x, i * bars_dist });
        std::string bar_text = "- " + std::to_string(1000/(7-i)) + "m";
        ImGui::Text("%s", bar_text.c_str());
    }
    ImGui::End();
}


void remove_background(const rs2::depth_frame& depth_frame, uint8_t* rgbatab, float depth_scale, int x_widthL,
	int x_widthR, int y_height, float clipping_distance, std::map<std::string, float>& measures)
{
	uint16_t* p_depth_frame = reinterpret_cast<uint16_t*>(const_cast<void*>(depth_frame.get_data()));
	int width = depth_frame.get_width();
	int height = depth_frame.get_height();
	int bpp = depth_frame.get_bytes_per_pixel();
	int top_height[2] = { 0,0 };

	int near_height[2] = { 0,0 };

	measures[closest_distance] = 0;
	measures[l_weight] = 0;
	measures[r_weight] = 0;

#pragma omp parallel for schedule(dynamic) //Using OpenMP to try to parallelise the loop
	for (int y = 0; y < height; y++)
	{
		auto depth_pixel_index = y * width;// +50;
		for (int x = 0; x < width; x++, ++depth_pixel_index)
		{
			auto pixels_distance = p_depth_frame[depth_pixel_index];
			// Get the depth value of the current pixel
			auto offset = depth_pixel_index;// *bpp;
			// Check if the depth value is invalid (<=0) or greater than the threashold
			if (x <= x_widthL || y <= y_height || x >= width - x_widthR) // ||  y >= height - y_height)// clipping_dist)
			{
				// Calculate the offset in other frame's buffer to current pixel
				// Set pixel to "background" color (0x999999)
				std::memset(&rgbatab[offset * 4 + 4], 0, 1);
			}

			else  if (pixels_distance <= 0.f || pixels_distance > clipping_distance)
			{
				std::memset(&rgbatab[offset * 4], 255, 1);
				std::memset(&rgbatab[offset * 4 + 1], 255, 1);
				std::memset(&rgbatab[offset * 4 + 2], 255, 1);
				std::memset(&rgbatab[offset * 4 + 3], 255, 1);

			}


			else
			{


				if (top_height[0] == 0)
				{
					top_height[0] = x;

					top_height[1] = y;
					measures[head_distance] = pixels_distance;
				}
				if (x <= top_height[0])
				{
					measures[l_weight] += pixels_distance;
				}
				else
				{

					measures[r_weight] += pixels_distance;
				}

				if (measures[closest_distance] > pixels_distance || measures[closest_distance] == 0)
				{
					measures[closest_distance] = pixels_distance;
					near_height[0] = x;
					near_height[1] = y;
				}
				int val = p_depth_frame[offset] / 1600.00 * 255;
				std::memset(&rgbatab[offset * 4], val, 1);
				std::memset(&rgbatab[offset * 4 + 1], val, 1);
				std::memset(&rgbatab[offset * 4 + 2], 255, 1);
				std::memset(&rgbatab[offset * 4 + 3], val, 1);
			}
		}
	}
	if (top_height[0] != 0)
	{
		draw_circle(top_height[0], top_height[1], rgbatab, width);
		draw_circle(near_height[0], near_height[1], rgbatab, width);
		draw_center_line(top_height[0], top_height[1], rgbatab, height, width);
	}

}

rs2_stream find_stream_to_align(const std::vector<rs2::stream_profile>& streams)
{
    //Given a vector of streams, we try to find a depth stream and another stream to align depth with.
    //We prioritize color streams to make the view look better.
    //If color is not available, we take another stream that (other than depth)
    rs2_stream align_to = RS2_STREAM_ANY;
    bool depth_stream_found = false;
    bool color_stream_found = false;
    for (rs2::stream_profile sp : streams)
    {
        rs2_stream profile_stream = sp.stream_type();
        if (profile_stream != RS2_STREAM_DEPTH)
        {
            if (!color_stream_found)         //Prefer color
                align_to = profile_stream;

            if (profile_stream == RS2_STREAM_COLOR)
            {
                color_stream_found = true;
            }
        }
        else
        {
            depth_stream_found = true;
        }
    }

    if(!depth_stream_found)
        throw std::runtime_error("No Depth stream available");

    if (align_to == RS2_STREAM_ANY)
        throw std::runtime_error("No stream found to align with Depth");

    return align_to;
}

bool profile_changed(const std::vector<rs2::stream_profile>& current, const std::vector<rs2::stream_profile>& prev)
{
    for (auto&& sp : prev)
    {
        //If previous profile is in current (maybe just added another)
        auto itr = std::find_if(std::begin(current), std::end(current), [&sp](const rs2::stream_profile& current_sp) { return sp.unique_id() == current_sp.unique_id(); });
        if (itr == std::end(current)) //If it previous stream wasn't found in current
        {
            return true;
        }
    }
	return false;
}

void draw_circle(int _x, int _y, uint8_t* rgbatab, int width)
{

	if (_y > 400)
	{
		return;
	}

	int r = 20;
	if (_x < r || _y < r || _x == 0)
	{
		return;
	}


	for (int y = _y - r; y < _y + r; y++)
	{
		for (int x = _x - r; x < _x + r; x++)
		{
			if ((x - _x) * (x - _x) +
				(y - _y) * (y - _y) <= r * r)
			{
				int pix = y * width + x;
				std::memset(&rgbatab[pix * 4], 0, 1);
				std::memset(&rgbatab[pix * 4 + 1], 255, 1);
				std::memset(&rgbatab[pix * 4 + 2], 0, 1);

				std::memset(&rgbatab[pix * 4 + 3], 255, 1);

			}


		}
	}
}
void draw_center_line(int _x, int _y, uint8_t* rgbatab, int height, int width)

{
	if (_x == 0)
	{
		return;
	}


	for (int y = _y; y < height; y++)
	{

		//        for (int i = 0; i < 4; i++)
		 //       {

		std::memset(&rgbatab[y * (width * 4) + _x * 4], 255, 1);
		std::memset(&rgbatab[y * (width * 4) + _x * 4 + 1], 0, 1);
		std::memset(&rgbatab[y * (width * 4) + _x * 4 + 2], 255, 1);
		std::memset(&rgbatab[y * (width * 4) + _x * 4 + 3], 255, 1);
		//      }
	}


}
void get_filter_config(float& x_widthL, float& x_widthR,float& y_height,float& clipping_distance)
{
	std::string line;
	std::ifstream myfile("config_frame.txt");
	if (myfile.is_open())
	{
		std::string conf_frame;
		while (getline(myfile, line))
		{
			conf_frame = line;
		}
		//    std::string str = "1,2,3,4,5,6";
		std::vector<int> vect;

		std::stringstream ss(conf_frame);

		for (int i; ss >> i;) {
			vect.push_back(i);
			if (ss.peek() == ',')
				ss.ignore();
		}

		//    for (std::size_t i = 0; i < vect.size(); i++)
		 //       std::cout << vect[i] << std::endl;
		x_widthL = vect[0];
		x_widthR = vect[1];
		y_height = vect[2];
		clipping_distance = vect[3];
		myfile.close();
	}


}
