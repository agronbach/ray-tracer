#include <stdio.h>
#include <glm/glm.hpp>
#include "image.h"

#define MAX_SPHERES 100
#define T_MAX 1e7

using namespace glm;

const int kResolutionX = 600;
const int kResolutionY = 600;
const int kDepth = 16;
const vec3 kBackgroundColor = vec3(0.0f, 0.0f, 0.0f);

void print(const vec3& v) {
   printf("(%f, %f, %f)\n", v.x, v.y, v.z);
}

typedef struct {
   vec3 bottom_left;
   vec3 top_right;
} ViewPlane;

typedef struct {
   ViewPlane view_plane;
   vec3 position;
   vec3 look_at;
   vec3 up;
} Camera;

typedef struct {
   vec3 origin;
   vec3 direction;
} Ray;

typedef struct {
   vec3 diffuse;
   vec3 specular;
   vec3 ambient;
   float alpha;
} Material;

typedef struct {
   vec3 center;
   float radius;

   Material material;
} Sphere;

typedef struct {
   vec3 position;
   Material material;
} Light;

// GLOBAL DATA /////////////////////////////////////////////////////////////////
vec3 g_pixel_buffer[kResolutionX][kResolutionY];
Sphere g_spheres[MAX_SPHERES];
int g_num_spheres = 0;
// GLOBAL DATA /////////////////////////////////////////////////////////////////

Ray ray_from_pixel_coordinates(int row, int col, Camera& camera) {
   Ray ray;
   ray.origin = camera.position;
   const float left = camera.view_plane.bottom_left.x;
   const float right = camera.view_plane.top_right.x;
   const float bottom = camera.view_plane.bottom_left.y;
   const float top = camera.view_plane.top_right.y;

   ray.direction.x = left + (right - left) * (row + 0.5f) / kResolutionX;
   ray.direction.y = bottom + (top - bottom) * (col + 0.5f) / kResolutionY;
   ray.direction.z = kDepth;
   return ray;
}


// Returns true if the ray hits |sphere| within [t0, t1].
// If true, populates |collision_t| with the smaller, positive 't' value of the collision.
bool does_ray_collide_with_sphere(const Ray& ray, Sphere* sphere, float t0, float t1, float *collision_t) {
   vec3 d = normalize(ray.direction);
   vec3 e = ray.origin;
   vec3 c = sphere->center;

   float discriminant = pow(dot(d, e-c), 2) - dot(d, d) * (dot(e-c, e-c) - pow(sphere->radius, 2));
   if (discriminant < 0.0f)
      return false;

   float term1 = dot(-d, e-c);
   float term2 = pow(discriminant, 0.5);
   float term3 = dot(d, d);

   float negative_root = (term1 - term2) / term3;
   float positive_root = (term1 + term2) / term3;

   *collision_t = negative_root > t0 ? negative_root : positive_root;
   return *collision_t > t0 && *collision_t < t1;
}

// Returns true if the ray hits any of |g_spheres| in the range t [0.0f, T_MAX]
// If true, populates |hit_sphere_index| with the index into |g_spheres| of the
// colliding sphere and |collision_t| with the 't' value of the collision.
bool test_for_hit(const Ray& ray, int *hit_sphere_index, float *collision_t, int ignore_sphere_index) {
   float t0 = 0.0f;
   float t1 = T_MAX;
   bool hit = false;

   int i;
   for (i = 0; i < g_num_spheres; ++i) {
      if (i == ignore_sphere_index)
         continue;
      if (does_ray_collide_with_sphere(ray, &g_spheres[i], t0, t1, collision_t)) {
         hit = true;
         *hit_sphere_index = i;
         t1 = *collision_t;
      }
   }

   *collision_t = t1;
   return hit;
}

void add_sphere_to_scene(const vec3& center, float radius, Material material) {
   g_spheres[g_num_spheres].center = center;
   g_spheres[g_num_spheres].material = material;
   g_spheres[g_num_spheres++].radius = radius;
}

vec3 reflection(const vec3& light_direction, const vec3& normal) {
   return normalize(normal * 2.0f*(dot(light_direction, normal)) - light_direction);
}

Ray make_shadow_feeler_ray(const vec3& position, const vec3& light_position) {
   Ray ray;
   ray.origin = position;
   ray.direction = light_position - position;
   return ray;
}

vec3 pixel_color(int sphere_index, const Light& light, const vec3& light_direction, const vec3& position, const vec3& view_direction) {
   vec3 color;
   vec3 normal = normalize(position - g_spheres[sphere_index].center);

   bool is_shadow = false;
   Ray shadow_feeler_ray(make_shadow_feeler_ray(position, light.position));
   int hit_sphere_index;
   float collision_t;
   if (test_for_hit(shadow_feeler_ray, &hit_sphere_index, &collision_t, sphere_index)) {
      is_shadow = true;
   }

   float reflection_component;
   float diffuse_component = dot(light_direction, normal);
   if (!is_shadow) {
      reflection_component = dot(reflection(light_direction, normal), view_direction);
      diffuse_component = dot(light_direction, normal);
   }


   for (int i = 0; i < 3; ++i) {
      float diffuse = 0.0f;
      float specular = 0.0f;
      if (!is_shadow && reflection_component > 0.0f)
         specular = light.material.specular[i]*g_spheres[sphere_index].material.specular[i] * pow(reflection_component, g_spheres[sphere_index].material.alpha);
      if (!is_shadow && diffuse_component > 0.0f)
         diffuse = light.material.diffuse[i]*g_spheres[sphere_index].material.diffuse[i] * diffuse_component;
      float ambient = light.material.ambient[i]*g_spheres[sphere_index].material.ambient[i];

      color[i] = ambient + diffuse + specular;
   }
   return color;
}

Light makeReddishLight() {
   Light light;
   light.material.diffuse = vec3(1);
   light.material.specular = vec3(.8);
   light.material.ambient = vec3(.8);
   light.position = vec3(0, 5, 10);
   return light;
}

void makeCameraViewPlane(Camera* camera) {
   // Compute u, v, w basis vectors
   vec3 w = normalize(camera->position - camera->look_at);
   vec3 u = normalize(cross(camera->up, w));
   vec3 v = normalize(cross(w, u));

   float left = 0.5f;
   float right = -0.5f;
   float top = 0.5f;
   float bottom = -0.5f;

   camera->view_plane.bottom_left = u*left + v*bottom - w + camera->position;
   camera->view_plane.top_right = u*right + v*top - w + camera->position;
}

Material makeDiffuseMaterial() {
   Material mat;
   mat.diffuse = vec3(.7, 0, 0);
   mat.specular = vec3(0, 0, 0.8);
   mat.ambient = vec3(0, .2, 0);
   mat.alpha = 20.0f;
   return mat;
}

Material makeSpecularMaterial() {
   Material mat;
   mat.diffuse = vec3(.8, 0, 0);
   mat.specular = vec3(0, .2, 0);
   mat.ambient = vec3(0, 0, .1);
   mat.alpha = 10.0f;
   return mat;
}

Material makeAmbientMaterial() {
   Material mat;
   mat.diffuse = vec3(.8, 0, 0);
   mat.specular = vec3(0, 0, 0.2);
   mat.ambient = vec3(.4, 0, .4);
   mat.alpha = 1.0f;
   return mat;
}

Camera makeDefaultCamera() {
   Camera camera;
   camera.position = vec3(0, 0, 0);
   camera.look_at = vec3(0, 5, 25);
   camera.up = vec3(0, 1, 0);
   makeCameraViewPlane(&camera);
   return camera;
}

void write_image_to_file() {
   Image image(kResolutionX, kResolutionY);
   for (int row = 0; row < kResolutionY; ++row)
      for (int col = 0; col < kResolutionX; ++col)
         image.pixel(row, col, g_pixel_buffer[row][col]);
   image.WriteTga("sphere.tga");
}

void calculate_pixel_buffer_from_ray_trace(Camera& camera, Light& light) {
   // for each pixel
   for (int row = 0; row < kResolutionY; ++row) {
      for (int col = 0; col < kResolutionX; ++col) {
         // compute viewing ray
         Ray ray(ray_from_pixel_coordinates(row, col, camera));
         int hit_sphere_index;
         float collision_t;
         if (test_for_hit(ray, &hit_sphere_index, &collision_t, -1)) {
            // find first object hit by ray and its surface normal n
            vec3 position = ray.origin + collision_t*normalize(ray.direction);
            vec3 light_direction = normalize(light.position - position);
            vec3 view_direction = normalize(camera.position - position);
            // set pixel color to value based on material, light, and n
            g_pixel_buffer[row][col] = pixel_color(hit_sphere_index, light, light_direction, position, view_direction);
         } else {
            // If no hit occurs, set to the background color
            g_pixel_buffer[row][col] = kBackgroundColor;
         }
      }
   }
}

int main(int argc, char** argv) {
   Camera camera = makeDefaultCamera();

   add_sphere_to_scene(vec3(.5, 0, 25), 0.6f, makeDiffuseMaterial());
   add_sphere_to_scene(vec3(0, .5, 23), 0.1f, makeSpecularMaterial());
   add_sphere_to_scene(vec3(-.5, -.5, 25), 0.4f, makeAmbientMaterial());

   Light light = makeReddishLight();

   calculate_pixel_buffer_from_ray_trace(camera, light);

   write_image_to_file();
}
