#include "lineshapes.h"

void Lines::add(GLfloat *vertex_data, GLfloat *color_data, size_t vertexCount, GLuint *indices_data, size_t indexCount)
{
    size_t point_size = points.size();
    size_t index_size = indices.size();

    points.resize(point_size + vertexCount);
    colors.resize(point_size + vertexCount);
    indices.resize(index_size + indexCount);

    for (size_t i = 0; i < vertexCount; ++i)
    {
        points[point_size + i].v[0] = vertex_data[i * 3];
        points[point_size + i].v[1] = vertex_data[i * 3 + 1];
        points[point_size + i].v[2] = vertex_data[i * 3 + 2];
    }
    for (size_t i = 0; i < vertexCount; ++i)
    {
        colors[point_size + i].v[0] = color_data[i * 3];
        colors[point_size + i].v[1] = color_data[i * 3 + 1];
        colors[point_size + i].v[2] = color_data[i * 3 + 2];
    }
    for (size_t i = 0; i < indexCount; ++i)
    {
        indices[index_size + i] = point_size + indices_data[i];
    }
}

void Lines::clear()
{
    points.clear();
    colors.clear();
    indices.clear();
}

void Lines::load_to_gpu()
{

    size_t vertex_count = points.size();
    size_t index_count = indices.size();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    size_t attribIx = 0;

    if (vertex_count)
    {
        glGenBuffers(1, &points_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
        glBufferData(GL_ARRAY_BUFFER, 3 * vertex_count * sizeof(GLfloat), &points[0].v[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(attribIx);
        glVertexAttribPointer(attribIx, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    ++attribIx;

    if (vertex_count)
    {
        glGenBuffers(1, &colors_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, colors_vbo);
        glBufferData(GL_ARRAY_BUFFER, 3 * vertex_count * sizeof(GLfloat), &colors[0].v[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(attribIx);
        glVertexAttribPointer(attribIx, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    }

    ++attribIx;

    if (index_count)
    {
        glGenBuffers(1, &index_vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_vbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * index_count, &indices[0], GL_STATIC_DRAW);
    }
    glBindVertexArray(0);
}

void Lines::get_shader_locations(GLuint shader_programme)
{
    model_matrix_location = glGetUniformLocation(shader_programme, "model");
}

void Lines::set_shader_data(GLuint shader_programme, const mat4 &referenceFrameMatrix)
{
    glUniformMatrix4fv(model_matrix_location, 1, GL_FALSE, referenceFrameMatrix.m);
}

void Lines::render(GLuint shader_programme)
{
    size_t index_count = indices.size();
    glBindVertexArray(vao);
    glDrawElements(GL_LINES, index_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Shapes::addArrow(Lines &lines, const vec3 &from, const vec3 &to, const vec3 &color)
{
    vec3 one(0.5054846f, 0.5068565f, 0.5046546f);

    const unsigned int iterations = 25;
    const unsigned int sharpness = 0.1f; // range [0,1] : 0 = sharpest, 1 = flattest
    const unsigned int length = 0.15f;

    const unsigned int total = iterations + 2;

    vec3 arrow_vertices[total];
    arrow_vertices[0] = from;
    arrow_vertices[1] = to;

    vec3 arrow_colors[total];
    for (unsigned int i = 0; i < total; i++)
        arrow_colors[i] = color;

    auto axis_dir = to - from;
    auto axis_inv = from - to;
    auto point = normalise(normalise(cross(one, axis_dir)) * sharpness + normalise(axis_inv) * 0.5f) * length;
    auto alpha = 360.f / iterations;

    // generate round points
    for (unsigned int i = 0; i < iterations; i++)
    {
        auto p4 = vec4(point, 1);
        auto r = quat_to_mat4(quat_from_axis_deg(alpha * i, axis_dir.x, axis_dir.y, axis_dir.z)) * p4;
        auto p = vec3(r.x, r.y, r.z);
        arrow_vertices[i + 2] = p + to;
    }

    // add from-to line
    const unsigned int arrow_indices_num = iterations * 2 + 2;
    unsigned int arrow_indices[arrow_indices_num];
    arrow_indices[0] = 0; // from
    arrow_indices[1] = 1; // to

    // add to-points lines
    for (unsigned int i = 0; i < iterations; i++)
    {
        arrow_indices[i * 2 + 2] = 1;     // to
        arrow_indices[i * 2 + 3] = i + 2; // point
    }

    lines.add(&arrow_vertices[0].v[0], &arrow_colors[0].v[0], total, &arrow_indices[0], arrow_indices_num);
}

// void Shapes::addArrow(Lines &lines, const vec3 &from, const vec3 &to, const vec3 &color)
//{
//     vec3 one(0.2354846f, 0.2168565f, 0.2516546f);
//     vec3 two(-0.2354846f, 0.2168565f, 0.2516546f);
//
//     auto sharpness = 0.1f; // range [0,1] : 0 = sharpest, 1 = flattest
//     auto length = 0.15f;
//
//     auto dir = to - from;
//     auto inv = from - to;
//
//     auto pv1 = normalise(normalise(cross(one, dir)) * sharpness + normalise(inv) * 0.5f) * length + to;
//     auto pv2 = normalise(normalise(cross(one, inv)) * sharpness + normalise(inv) * 0.5f) * length + to;
//     auto pv3 = normalise(normalise(cross(two, dir)) * sharpness + normalise(inv) * 0.5f) * length + to;
//     auto pv4 = normalise(normalise(cross(two, inv)) * sharpness + normalise(inv) * 0.5f) * length + to;
//
//     vec3 arrow_vertices[] = {
//         from, to, pv1, pv2, pv3, pv4,
//     };
//
//     vec3 arrow_colors[] = {
//         color, color, color, color, color, color,
//     };
//
//     // each number is a vertex, each pair is a line
//     unsigned int arrow_indices[] = {0, 1, 1, 2, 1, 3, 1, 4, 1, 5};
//
//     lines.add(&arrow_vertices[0].v[0], &arrow_colors[0].v[0], 6, &arrow_indices[0], 10);
// }

void Shapes::addGrid(Lines &lines, const vec3 &from, const vec3 &to, const vec3 &color, int divs)
{
    int steps = divs + 1;

    vec3 size = to - from;
    float x = size.v[0] / (steps - 1);
    float y = size.v[2] / (steps - 1);

    std::vector<vec3> vertices;
    vertices.resize(steps * steps);
    std::vector<vec3> colors;
    colors.resize(steps * steps);

    for (int i = 0; i < steps; ++i)
    {
        for (int j = 0; j < steps; ++j)
        {
            vertices[j + i * steps] = vec3(from.v[0] + j * x, 0.f, from.v[2] + i * y);
            colors[j + i * steps] = color;
        }
    }

    std::vector<unsigned int> indices;
    indices.resize(4 * steps);
    for (int i = 0; i < steps; ++i)
    {
        indices[i * 2] = i;
        indices[i * 2 + 1] = i + steps * (steps - 1);
    }
    for (int i = 0; i < steps; ++i)
    {
        indices[steps * 2 + i * 2] = i * steps;
        indices[steps * 2 + i * 2 + 1] = i * steps + (steps - 1);
    }

    lines.add(&vertices[0].v[0], &colors[0].v[0], vertices.size(), &indices[0], indices.size());
}
