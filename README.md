# glPBRSampleViewer
![img1](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/51ccb9e8-7d96-45d1-936c-e83ebc25063f)

PBR + IBL based gltf viewer in OpenGL 4.6. Also generates Pre filtered enviornment maps for diffuse and specular IBL as well as generates cubemaps for skybox from spherical HDR maps.\
\
Use the **"Display"** drop down menu on the top left corner to view the controls and interact with the application.\
\
You can just directly run the compiled exe by downloading the **glPBRSampleViewer.zip** from the release section if you dont wish to compile. Although if you do please compile it under the x64 Release flag and copy all the contents of the bin folder into the local exe folder.

![8](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/212998d7-4f47-4778-b2d1-83944e421568)

# Rendering Features
* PBR Cook-Torrance Microfacet specular BRDF (Lambertian diffuse BRDF + F(Schlick)* D(GGX) * G(Smith)G1(Schlick_GGX)) 
* Importance sampling and Halton low discrepancy sequence for generating diffuse and specular pre filtered enviornment maps for IBL.\
  [Special thanks to the GSN Composer youtube channel for these PBR and IBL topics](https://www.youtube.com/@gsn-composer).
  
* Direct Drawing with texture arrays (Avoided indirect drawing with glMultiDrawElementsIndirect() since that would require bindless textures and many systems dont support that)
* Alpha blending (Transparent materials are drawn last)
* As well as other minor features such as  gamma correction, spherical to cubemap conversion, and multisampling.

![channels](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/a7272e71-9acf-4821-84dd-0cedac2aef07)


## Application Settings
If you wish to change application settings such as window resolution, enabling fullscreen or enabling multisampling etc., just open the **data.json** file and directly change the first few variable values.\
MSAA samples are set to 8 which can hinder performance on a slow machine, so either disable MSAA or change the "Samples" var in data.json.\
Also, please only use gltf models as well as 1k resolution HDRi maps for consistent performance. If you want to add your own models and HDR maps, just add thier names in the sequence displayed in the **data.json** file.

https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/1c0c9c29-c7cb-42f5-bafa-1efb34bad879

# More Screenshots
![1](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/d7c3a1ae-fde4-4233-adcf-d7c3f4f78a1d)
![2](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/301c4382-abab-4b81-bd23-2f9413b03541)
![3](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/6c4d5f84-b2ba-4c60-81d8-7576fd34a82f)
![4](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/a5fd128e-9148-4dd8-acd5-2898b9ee7da4)
![5](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/f3b6de12-cc2a-4a4c-bc85-eaf17c4e387a)
![6](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/3582dafe-290f-40e6-ab9b-5f4522017c9a)
![7](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/23280619-ed4e-47c4-9a93-865d8e599211)
![9](https://github.com/chirag9510/glPBRSampleViewer/assets/78268919/3077f867-9d91-484a-8e6b-ad5b9e82f7d5)

# References
* [GSN Composer Youtube Channel](https://www.youtube.com/@gsn-composer) and thier Shaders Monthly series of videos specifically on the PBR and IBL topics.
* [glTF-Sample-Viewer](https://github.com/KhronosGroup/glTF-Sample-Viewer) for Spherical Map to Cubemap conversion shader fundamentals.
* [KhronosGroup/glTF-Sample-Models](https://github.com/KhronosGroup/glTF-Sample-Models) and [Poly Haven](https://polyhaven.com/) for the GLTF models and HDR maps.
* Also **Libraries** used Assimp, glm, glew, imgui, JSON, spdlog, stb_image, SDL2.
