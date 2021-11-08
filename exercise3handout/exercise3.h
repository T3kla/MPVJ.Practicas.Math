#pragma once

#ifdef WIN32
#include <direct.h>
#endif
#include <array>
#include <assert.h>

#include "camera.h"
#include "gl_utils.h"
#include "lineshapes.h"
#include "maths_funcs.h"
#include "mesh.h"
#include "node.h"

constexpr int NumSpheres = 4;

struct Exercise3
{

    GLFWwindow *window = NULL;

    static bool isInputEnabled;
    static double mousePosX, mousePosY;
    static double prevMousePosX, prevMousePosY;
    int windowsWidth, windowsHeight;

    static void onWindowsFocus(GLFWwindow *window, int focused)
    {
        if (focused)
        {
            // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwPollEvents();
            glfwGetCursorPos(window, &prevMousePosX, &prevMousePosY);
            isInputEnabled = true;
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            isInputEnabled = false;
        }
    }

    // ------------------------------------------------------------------------------------------ REVIEW

    static vec3 getWorldMousePosition(float _mouseX, float _mouseY, float _wWidth, float _wHeight, const mat4 &_projMat,
                                      const mat4 &_viewMat, const vec3 &_camPos)
    {
        // General info on the proj-view-model concept
        // https://stackoverflow.com/questions/46749675/opengl-mouse-coordinates-to-space-coordinates

        // Ray from mouse and collision with plane and sphere
        // https://antongerdelan.net/opengl/raycasting.html

        auto mouse = vec3(_mouseX, _mouseY, 0.);

        // Mouse coords to Normalised Device Coordinates, center of the screen is (0,0), edges -1 to 1
        auto mouse_ndc = vec3((mouse.x * 2.f) / _wWidth - 1.f, 1.f - (mouse.y * 2.f) / _wHeight, -1.f);

        // NDC to clip, inside the frustum cube, frustum cube is origin
        auto mouse_clip = vec4(mouse_ndc.x, mouse_ndc.y, -1.f, 1.f); // negative z should be forwards

        // Clip to eye, in between 'near' and 'far', camera is origin
        auto mouse_eye = inverse(_projMat) * mouse_clip;
        mouse_eye = vec4(mouse_eye.x, mouse_eye.y, -1.f, 0.f);

        // Eye to world, in world space, root is origin
        auto mouse_world = vec3(inverse(_viewMat) * mouse_eye);

        return normalise(mouse_world) + _camPos;
    }

    // ------------------------------------------------------------------------------------------

    struct Ray
    {
        vec3 origin;
        vec3 direction;
        Ray(vec3 origin = vec3(0, 0, 0), vec3 direction = vec3(0, 0, 0)) : origin(origin), direction(direction)
        {
        }
    };

    // Check if a ray and a sphere intersect.
    // It rejects intersections behind the ray origin.
    // Sets intersection_distance to the closest intersection.
    static bool raySphereIntersection(const Ray &_ray, vec3 _sphere_pos, float _sphere_radius, float *distance_)
    {
        assert(fabsf(length(_ray.direction) - 1) < 1e-03);

        auto A_C = _ray.origin - _ray.direction;
        auto a = 1.f;
        auto b = 2 * dot(_ray.direction, A_C);
        auto c = dot(A_C, A_C) - _sphere_radius * _sphere_radius;
        auto discriminant = b * b - 4 * a * c;

        // Ray hits no points
        if (discriminant < 0.f)
            return false;

        // Ray hits one point
        if (0.f == discriminant)
        {
            auto hit = -b + sqrtf(discriminant);

            if (hit < 0.f)
                return false;

            *distance_ = hit;

            return true;
        }

        // Ray hits two points
        if (discriminant > 0.f)
        {
            auto sqrt_discriminant = sqrtf(discriminant);

            auto hit_near = -b - sqrt_discriminant;
            auto hit_far = -b + sqrt_discriminant;

            // Sphere is behind
            if (hit_near < 0.f && hit_far < 0.f)
                return false;

            // Sphere is ahead
            if (hit_near > 0.f && hit_far > 0.f)
                *distance_ = hit_near;

            // Origin is inside the sphere
            if (hit_near < 0.f && hit_far > 0.f)
                *distance_ = 0.f;

            return true;
        }

        return false;
    }

    // ------------------------------------------------------------------------------------------ REVIEW

    // Check if a ray pass through a sphere.
    // It rejects intersections behind the ray origin.
    static bool raySphereThrough(const Ray &_ray, vec3 _sphere_pos, float _sphere_radius)
    {
        // Closest distance from point to ray
        // a = point - orig
        // dis = |a x b| when b is unitary

        auto to_sphere = _sphere_pos - _ray.origin;
        auto distance = length(cross(to_sphere, _ray.direction));
        auto frontfacing = dot(to_sphere, _ray.origin) > 0.f;

        return frontfacing && distance < _sphere_radius;
    }

    // ------------------------------------------------------------------------------------------

    // as in
    // https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
    // also "Ray Sphere Intersection 2 Geometrical.pdf"

    static void onKeyPressed(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        Exercise3 &exercise = *static_cast<Exercise3 *>(glfwGetWindowUserPointer(window));

        if (key != GLFW_KEY_F)
            return;

        auto cam = vec3(exercise.camNode.worldMatrix.getColumn(3));
        auto mouse =
            getWorldMousePosition(static_cast<float>(mousePosX), static_cast<float>(mousePosY),
                                  static_cast<float>(exercise.windowsWidth), static_cast<float>(exercise.windowsHeight),
                                  exercise.camera.proj_mat, exercise.camNode.worldInverseMatrix, cam);

        auto ray = Ray(mouse, normalise(mouse - cam));

        exercise.axis.clear();
        Shapes::addArrow(exercise.axis, ray.origin, ray.origin + normalise(ray.direction), vec3(1, 0, 0));
        exercise.axis.load_to_gpu();
    }

    /* this function is called when the mouse buttons are clicked or un-clicked */
    static void onMouseClicked(GLFWwindow *window, int button, int action, int mods)
    {
        Exercise3 &exercise = *static_cast<Exercise3 *>(glfwGetWindowUserPointer(window));

        if (GLFW_PRESS == action)
        {
            auto cam = vec3(exercise.camNode.worldMatrix.getColumn(3));
            auto mouse = getWorldMousePosition(static_cast<float>(mousePosX), static_cast<float>(mousePosY),
                                               static_cast<float>(exercise.windowsWidth),
                                               static_cast<float>(exercise.windowsHeight), exercise.camera.proj_mat,
                                               exercise.camNode.worldInverseMatrix, cam);

            auto ray = Ray(mouse, normalise(mouse - cam));

            auto closest_sphere = -1;
            auto closest_sphere_dis = 0.f;

            for (int i = 0; i < NumSpheres; i++)
            {
                auto dis = 0.f;
                auto pos = vec3(exercise.sphereNodes[i].worldMatrix.getColumn(3));
                // auto this_works = raySphereThrough(ray, spherePos, 1);

                if (raySphereIntersection(ray, pos, 1, &dis))
                    if (closest_sphere == -1 || dis < closest_sphere_dis)
                    {
                        closest_sphere = i;
                        closest_sphere_dis = dis;
                    }
            }

            exercise.selectedSphereIndex = closest_sphere;
            printf("sphere %i was clicked\n", closest_sphere);
        }
    }

    vec3 ambientColor = vec3(0.7f, 0.7f, 0.75f);

    Meshgroup meshGroup;
    std::vector<Meshgroup::Mesh *> gulls;

    std::array<Node, NumSpheres> sphereNodes;
    std::array<vec3, NumSpheres> spherePositions = {vec3(0, 0, 1), vec3(2, 0, 0), vec3(-2, 0, 0), vec3(-2, 0, -2)};
    std::array<vec3, NumSpheres> sphereScales = {vec3(1, 1, 1), vec3(1, 1, 1), vec3(1, 1, 1), vec3(1, 1, 1)};
    std::array<vec3, NumSpheres> sphereColor = {vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1), vec3(1, 1, 0)};
    int selectedSphereIndex = NumSpheres + 1;

    Node meshGroupNode;
    float meshYaw = 0;
    // float meshYaw2 = 0;

    float camYaw = 0.f;
    float camPitch = 0.f;

    Camera camera;
    Node camNode;
    vec3 cameraPosition;

    Node sceneRoot;
    GLuint mesh_shader_index;
    GLuint lines_shader_index;

    Lines grid;
    Lines axis;

    void init(int width, int height)
    {

        windowsWidth = width;
        windowsHeight = height;

        isInputEnabled = true;

        // init
        // restart_gl_log();
        startGlContext(&window, width, height);
        glfwSetWindowUserPointer(window, this);

        mesh_shader_index = create_programme_from_files("test_vs.glsl", "test_fs.glsl");
        lines_shader_index = create_programme_from_files("lines_vs.glsl", "lines_fs.glsl");

        sceneRoot.init();

        meshGroupNode.init();
        sceneRoot.addChild(meshGroupNode);

        for (int i = 0; i < NumSpheres; ++i)
        {
            sphereNodes[i].init();
            sphereNodes[i].position = spherePositions[i];
            meshGroupNode.addChild(sphereNodes[i]);
        }

        // float scaleValue = 0.001f;
        // meshGroupNode.scale = vec3(scaleValue, scaleValue, scaleValue);
        // meshGroupNode.position = vec3(0, 20, 0);

        _chdir("../data/");
        Meshgroup::load_default_textures();

        //_chdir("../data/DamagedHelmet/");
        // meshGroup.load_from_file("DamagedHelmet.gltf");

        //_chdir("../data/tiki_treasure/");
        // meshGroup.load_from_file("scene.gltf");

        _chdir("../data/sphere/");
        meshGroup.load_from_file("sphere.obj");
        meshGroup.load_to_gpu();
        meshGroup.get_shader_uniforms(mesh_shader_index);

        assert(meshGroup.nodes.size() > 0);
        assert(meshGroup.meshes.size() > 0);

        meshGroupNode.addChild(meshGroup.nodes[0]);

        vec3 gridColor(fmodf(ambientColor.v[0] + 0.5f, 1.f), fmodf(ambientColor.v[1] + 0.5f, 1.f),
                       fmodf(ambientColor.v[2] + 0.5f, 1.f));
        Shapes::addGrid(grid, vec3(-5, 0, -5), vec3(5, 0, 5), gridColor, 10);
        grid.load_to_gpu();
        grid.get_shader_uniforms(lines_shader_index);

        // camera
        cameraPosition = vec3(0, 1, 6);

        camNode.init();
        camNode.position = cameraPosition;
        sceneRoot.addChild(camNode);

        camera.near = 0.1f;
        camera.far = 1000.f;
        camera.fov = 67.0f * ONE_DEG_IN_RAD;                    // convert degrees to radians
        camera.aspect = (float)g_gl_width / (float)g_gl_height; // aspect ratio

        camera.updateProjection();
        camera.speed = 7.0f;
        camera.yaw_speed = 20.f;
        camera.pitch_speed = 10.f;

        sceneRoot.updateHierarchy();

        // tell GL to only draw onto a pixel if the shape is closer to the viewer
        glEnable(GL_DEPTH_TEST); // enable depth-testing
        glDepthFunc(GL_LESS);    // depth-testing interprets a smaller value as "closer"
        glEnable(GL_CULL_FACE);  // cull face
        glCullFace(GL_BACK);     // cull back face
        glFrontFace(GL_CCW);     // GL_CCW for counter clock-wise

        glfwPollEvents();
        glfwGetCursorPos(window, &prevMousePosX, &prevMousePosY);
        // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetWindowFocusCallback(window, onWindowsFocus);
        glfwSetMouseButtonCallback(window, onMouseClicked);
        glfwSetKeyCallback(window, onKeyPressed);
    }

    void update()
    {
        static float previous_seconds = static_cast<float>(glfwGetTime());
        float current_seconds = static_cast<float>(glfwGetTime());
        float elapsed_seconds = current_seconds - previous_seconds;
        previous_seconds = current_seconds;

        _update_fps_counter(window);
        // wipe the drawing surface clear
        glClearColor(ambientColor.v[0], ambientColor.v[1], ambientColor.v[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, g_gl_width, g_gl_height);
        glfwPollEvents();

        // --------------------------------------------------------------------------- REVIEW
        if (isInputEnabled)
        {
            glfwGetCursorPos(window, &mousePosX, &mousePosY);

            auto mouseDeltaX = static_cast<float>(mousePosX - prevMousePosX);
            auto mouseDeltaY = static_cast<float>(mousePosY - prevMousePosY);

            prevMousePosX = mousePosX;
            prevMousePosY = mousePosY;

            camYaw += -mouseDeltaX * camera.yaw_speed * elapsed_seconds;
            camPitch += -mouseDeltaY * camera.pitch_speed * elapsed_seconds;
        }

        const auto PitchLimit = 80.f;

        camPitch = camPitch > PitchLimit ? PitchLimit : camPitch;
        camPitch = camPitch < -PitchLimit ? -PitchLimit : camPitch;
        camYaw = fmodf(camYaw, 360);

        const float r = (355.f / 113.f) / 180.f;
        auto deg2rad = [&](float &deg) { return deg * r; };

        auto yaw = deg2rad(camYaw);
        auto pitch = deg2rad(camPitch);

        vec3 wUp(0, 1, 0);

        // Look vector from Yaw and Pitch
        vec3 fforward = normalise(vec3(cosf(pitch) * cosf(yaw), sinf(pitch), cosf(pitch) * sinf(-yaw)));

        // The object and the view are not alineated so forward is
        // looking right. The following code corrects that.
        auto t1 = vec4(fforward, 1);
        auto t2 = quat_to_mat4(quat_from_axis_deg(90.f, wUp.x, wUp.y, wUp.z)) * t1;
        auto forward = vec3(t2.x, t2.y, t2.z);
        auto right = normalise(cross(forward, wUp));

        // auto Y = quat_from_axis_rad(yaw, wUp.x, wUp.y, wUp.z);
        // auto P = quat_from_axis_rad(pitch, right.x, right.y, right.z);

        // camNode.rotation = P * Y;

        camNode.rotation = quat_from_axis_deg(camYaw, 0, 1, 0) * quat_from_axis_deg(camPitch, 1, 0, 0);

        if (glfwGetKey(window, GLFW_KEY_W))
            cameraPosition += forward * elapsed_seconds * camera.speed;
        if (glfwGetKey(window, GLFW_KEY_S))
            cameraPosition -= forward * elapsed_seconds * camera.speed;
        if (glfwGetKey(window, GLFW_KEY_A))
            cameraPosition -= right * elapsed_seconds * camera.speed;
        if (glfwGetKey(window, GLFW_KEY_D))
            cameraPosition += right * elapsed_seconds * camera.speed;
        if (glfwGetKey(window, GLFW_KEY_SPACE))
            cameraPosition += wUp * elapsed_seconds * camera.speed;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
            cameraPosition -= wUp * elapsed_seconds * camera.speed;

        camNode.position = cameraPosition;

        //  ---------------------------------------------------------------------------

        if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(window, 1);

        mat4 cameraMatrix = translate(identity_mat4(), cameraPosition * -1.f);
        mat4 gridMatrix = translate(identity_mat4(), vec3(0, 0, 0));

        meshGroupNode.rotation = quat_from_axis_deg(meshYaw += elapsed_seconds * 10, 0, 1, 0);

        sceneRoot.updateHierarchy();

        glUseProgram(mesh_shader_index);

        camera.get_shader_uniforms(mesh_shader_index);
        camera.set_shader_uniforms(mesh_shader_index, camNode.worldInverseMatrix);
        // camera.set_shader_uniforms(mesh_shader_index, cameraMatrix );

        meshGroup.set_shader_uniforms(mesh_shader_index, ambientColor);

        for (int i = 0; i < NumSpheres; ++i)
        {
            meshGroup.meshes[0].render(mesh_shader_index, sphereNodes[i].worldMatrix,
                                       i == selectedSphereIndex ? vec3(1, 1, 1) : sphereColor[i]);
        }

        glUseProgram(0);

        glUseProgram(lines_shader_index);

        camera.get_shader_uniforms(lines_shader_index);
        camera.set_shader_uniforms(lines_shader_index, camNode.worldInverseMatrix);
        // camera.set_shader_uniforms(mesh_shader_index, cameraMatrix );

        grid.get_shader_uniforms(lines_shader_index);
        grid.set_shader_uniforms(lines_shader_index, sceneRoot.worldMatrix);
        // grid.set_shader_uniforms(lines_shader_index, gridMatrix);
        grid.render(lines_shader_index);

        axis.get_shader_uniforms(lines_shader_index);
        axis.set_shader_uniforms(lines_shader_index, sceneRoot.worldMatrix);

        axis.render(lines_shader_index);

        glUseProgram(0);

        // put the stuff we've been drawing onto the display
        glfwSwapBuffers(window);
    }

    bool isLoopGo()
    {
        return !glfwWindowShouldClose(window);
    }

    void terminate()
    {
        // close GL context and any other GLFW resources
        glfwTerminate();
    }
};
