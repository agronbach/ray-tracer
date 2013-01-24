#include <stdio.h>
#include <glm/glm.hpp>
#include "image.h"

#define MAX_SPHERES 10
#define T_MAX 10000

using namespace glm;

const int kResolutionX = 600;
const int kResolutionY = 600;
const int kDepth = 16;
const vec3 backgroundColor = vec3(0.0f, 0.0f, 0.0f);

void print(const vec3& v) {
   printf("(%f, %f, %f)\n", v.x, v.y, v.z);
}

vec3 g_pixel_buffer[kResolutionX][kResolutionY];

typedef struct {
   vec3 bottom_left;
   vec3 top_right;
} ViewPlane;

typedef struct {
   ViewPlane view_plane;
   vec3 position;
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

Sphere g_spheres[MAX_SPHERES];
int g_num_spheres = 0;

Ray ray_from_pixel_coordinates(int row, int col, Camera& camera) {
   Ray ray;
   ray.origin = camera.position;
   const float l = camera.view_plane.bottom_left.x;
   const float r = camera.view_plane.top_right.x;
   const float b = camera.view_plane.bottom_left.y;
   const float t = camera.view_plane.top_right.y;
   ray.direction.x = l + (r - l) * (row + 0.5f) / kResolutionX;
   ray.direction.y = b + (t - b) * (col + 0.5f) / kResolutionY;
   ray.direction.z = kDepth;
   return ray;
}

bool does_ray_collides_with_sphere(Ray* ray, Sphere* sphere, float t0, float t1, float *collision_t) {
   vec3 d = normalize(ray->direction);
   vec3 e = ray->origin;
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

bool test_for_hit(Ray* ray, int *hit_sphere_index, float *collision_t) {
   float t0 = 0.0f;
   float t1 = T_MAX;
   bool hit = false;

   int i;
   for (i = 0; i < g_num_spheres; ++i) {
      if (does_ray_collides_with_sphere(ray, g_spheres + i, t0, t1, collision_t)) {
         hit = true;
         *hit_sphere_index = i;
         t1 = *collision_t;
      }
   }

   *collision_t = t1;
   return hit;
}

void add_sphere(const vec3& center, float radius, Material material) {
   g_spheres[g_num_spheres].center = center;
   g_spheres[g_num_spheres].material = material;
   g_spheres[g_num_spheres++].radius = radius;
}

vec3 reflection(vec3* light_direction, vec3* normal) {
   return normalize(*normal * 2.0f*(dot(*light_direction, *normal)) - *light_direction);
}

vec3 pixel_color(Sphere* sphere, Light* light, vec3* light_direction, vec3* normal, vec3* view_direction) {
   vec3 color;
   float reflection_component = dot(reflection(light_direction, normal), *view_direction);
   float diffuse_component = dot(*light_direction, *normal);
   for (int i = 0; i < 3; ++i) {
      float diffuse = 0.0f;
      float specular = 0.0f;
      if (reflection_component > 0.0f)
         specular = light->material.specular[i]*sphere->material.specular[i] * pow(reflection_component, sphere->material.alpha);
      if (diffuse_component > 0.0f)
         diffuse = light->material.diffuse[i]*sphere->material.diffuse[i] * diffuse_component;
      float ambient = light->material.ambient[i]*sphere->material.ambient[i];

      color[i] = ambient + diffuse + specular;
   }
   return color;
}

Light makeWhiteLight() {
   Light light;
   light.material.diffuse = vec3(1);
   light.material.specular = vec3(1);
   light.material.ambient = vec3(1);
   light.position = vec3(-10, 15, 0);
   return light;
}

void makeCameraViewPlane(Camera* camera) {
   camera->view_plane.bottom_left = vec3(-.5, -.5, 1.0);
   camera->view_plane.top_right = vec3(.5, .5, 1.0);
}

int main(int argc, char** argv) {
   Camera camera;
   makeCameraViewPlane(&camera);

   Material diffuse;
   diffuse.diffuse = vec3(.8, 0, 0);
   diffuse.specular = vec3(0, .2, 0);
   diffuse.ambient = vec3(0, 0, .1);
   diffuse.alpha = 10.0f;

   Material specular;
   specular.diffuse = vec3(.7, 0, 0);
   specular.specular = vec3(0, 0, 0.8);
   specular.ambient = vec3(0, .1, 0);
   specular.alpha = 20.0f;

   Material ambient;
   ambient.diffuse = vec3(.8, 0, 0);
   ambient.specular = vec3(0, 0, 0.2);
   ambient.ambient = vec3(.4, 0, .4);
   ambient.alpha = 1.0f;

   add_sphere(vec3(.5, 0, 25), 0.6f, diffuse);
   add_sphere(vec3(0, .5, 25), 0.3f, specular);
   add_sphere(vec3(-.5, -.5, 25), 0.4f, ambient);

   Light light = makeWhiteLight();

   // Compute u, v, w basis vectors
   // for each pixel do
   for (int row = 0; row < kResolutionY; ++row) {
      for (int col = 0; col < kResolutionX; ++col) {
         // compute viewing ray
         Ray ray = ray_from_pixel_coordinates(row, col, camera);
         int hit_sphere_index;
         float collision_t;
         if (test_for_hit(&ray, &hit_sphere_index, &collision_t)) {
            // find first object hit by ray and its surface normal n
            vec3 position = ray.origin + collision_t*normalize(ray.direction);
            vec3 normal = normalize(position - g_spheres[hit_sphere_index].center);
            // set pixel color to value based on material, light, and n
            vec3 light_direction = normalize(light.position - position);
            vec3 view_direction = normalize(camera.position - position);
            g_pixel_buffer[row][col] = pixel_color(g_spheres + hit_sphere_index, &light, &light_direction, &normal, &view_direction);
         } else {
            g_pixel_buffer[row][col] = backgroundColor;
         }
      }
   }

   Image image(kResolutionX, kResolutionY);
   for (int row = 0; row < kResolutionY; ++row) {
      for (int col = 0; col < kResolutionX; ++col) {
         image.pixel(row, col, g_pixel_buffer[row][col]);
      }
   }
   image.WriteTga("sphere.tga");
}
