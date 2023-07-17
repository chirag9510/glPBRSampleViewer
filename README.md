# glPBRSampleViewer
![img1](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/9a5ba486-8906-4c59-8357-7bc5228d9b01)

PBR + IBL based gltf viewer in OpenGL 4.6. Also generates Pre filtered enviornments maps for diffuse and specular IBL as well as generates cubemaps for skybox from spherical HDR image.\
You can just directly download the exe from the release section if you dont wish to compile. Although if you do please compile it under the x64 Release flag and copy all the contents of the bin folder into the local exe folder.

# Rendering Features
* PBR Cook-Torrance Microfacet specular BRDF (Lambertian diffuse BRDF + F(Schlick)* D(GGX) * G(Smith)G1(Schlick_GGX))
* Importance sampling and Halton low discrepancy sequence for generating diffuse and specular pre filtered enviornment maps for IBL
* Direct Drawing with texture arrays (Avoided indirect drawing with glMultiDrawElementsIndirect() since that would require bindless textures and many systems dont support that)
* Alpha blending (Transparent materials are drawn last)
As well as other minor features such as  gamma correction, spherical to cubemap conversion, and multisampling.

![channels](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/c85b32e0-f4aa-498e-ad9c-5e376280de4f)

If you wish to change application settings such as window resolution, enabling fullscreen or enabling multisampling etc., just open the **data.json** file and directly change the first few values. MSAA samples are set to 8 which can hinder performance on a slow machine, so either disable MSAA or change the "Samples" var in data.json.
Also, please only use gltf models as well as 1k resolution HDRi maps for consistent performance if you want to add your own models and HDR maps.

# More Screenshots
![1](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/43f149f4-bffb-4b51-9951-5e8a6e1c719d)
![2](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/2bde1bc5-74e1-4a2f-91ce-7193ee3a4636)
![3](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/5dc7b285-6094-4dc1-b748-f3a122bdbbef)
![4](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/52d22637-e43e-40bd-9b7b-82289455f795)
![5](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/8d3dcedf-84ee-4c52-8665-4a9d47eb7782)
![6](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/bd8c4408-b159-4134-b1d5-5dd0d262a59c)
![7](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/d1e48cc0-0b99-43e1-9636-0d24aba40795)

Assimp,imgui,JSON,spdlog, stb_image, SDL2

# References
GSN composer for the PBR and IBL tutorials
gltfSampleViewer for panoramaToCubemap conversion shader (also wiki)
gltfSample models, polyhaven and 1 dude from sketchfab whose license is included
