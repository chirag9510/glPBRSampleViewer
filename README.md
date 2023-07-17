# glPBRSampleViewer

PBR + IBL based gltf viewer in OpenGL 4.6. Also generates Pre filter enviornments maps for diffuse and specular IBL as well as generates cubemaps for skybox from spherical HDR image.

Features
PBR Cook-Torrance Microfacet BRDF (Lambertian diffuse + F(Schlick)* D(GGX) * G(Smith)G1(Schlick_GGX))
Importance sampling and Halton low discrepancy sequence for generating diffuse and specular pre filter enviornment maps for IBL
Direct Drawing with texture arrays (Avoided indirect drawing with glMultiDrawElementsIndirect() since that would require bindless textures and I wish to run this on intel GPUs)
As well as minor features such as assimp loading, spherical to cubemap convertor, gamma correction and multisampling.

If you wish to change application settings such as window resolution, enabling fullscreen or enabling multisampling etc., just open the data.json file and directly the first few values.
Also, please only use gltf models as well as 1k resolution HDRi maps for consistent performance.

Assimp
imgui
JSON
spdlog
stb_image
SDL2

References
GSN composer for the PBR and IBL tutorials
gltfSampleViewer for panoramaToCubemap conversion shader (also wiki)
gltfSample models, polyhaven and 1 dude from sketchfab whose license is included
