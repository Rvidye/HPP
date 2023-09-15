#include <nanovdb/util/IO.h> // this is required to read (and write) NanoVDB files on the host
#include <iomanip>
#include <numeric>
#include "StringUtils.h"
#include "Renderer.h"
#include "Viewer.h"

// int main(int argc,char* argv[]){

//     std::string                                         platformName;
//     std::vector<std::pair<std::string, GridAssetUrl>>   urls;

//     for(int i = 1; i < argc; ++i){

//         std::string arg = argv[i];
//         if(!arg.empty()){

//             std::string urlStr = arg;
//             std::string nodeName = "";
//             auto pos = arg.find('=');
//             if(pos != std::string::npos){
//                 urlStr = arg.substr(pos+1);
//                 nodeName = arg.substr(0,pos);
//             }

//             GridAssetUrl url(urlStr);
//             urls.push_back(std::make_pair(nodeName, url));

//             if(url.isSequence()){
//                 // update frames start and end data
//             }
//         }
//     }

//     if(urls.size() > 0){
//         std::map<std::string, std::vector<GridAssetUrl>> nodeGridMap;
//         for(auto& nodeUrlPairs : urls){
//             nodeGridMap[nodeUrlPairs.first].push_back(nodeUrlPairs.second);
//         }

//         for(auto &it : nodeGridMap){
//             if(it.first.length()){
//                 int attachmentIndex = 0;
//                 //
//                 for(size_t i = 0; i < it.second.size(); i++)
//                 {
//                     auto& assetUrl = it.second[i];
//                     std::cout<<assetUrl.fullname()<<std::endl;
//                     if(assetUrl.scheme() == "file" && assetUrl.gridName().empty()){
//                         //get grid names from file
//                         std::string gridName;
//                         std::cout<<assetUrl.gridName();
//                         //add grids to scene or renderer
//                         // set scene node grid attachment
//                     }else {
                        
//                     }
//                 }
//             }else {
//                 //create scene node for each grid
//             }
//         }
//             // set selectSceneNodeByIndex
//             // reset Camera
//     }

//     // open window
//     // run()
//     // close

//     return EXIT_SUCCESS;
// }
int main(int argc, char* argv[])
{
    std::string                                       platformName;
    std::vector<std::pair<std::string, GridAssetUrl>> urls;
    RendererParams                                    rendererParams;

    bool batch = false;

    // make an invalid range.
    rendererParams.mFrameStart = 0;
    rendererParams.mFrameEnd = -1;

    StringMap renderStringParams;

    // make an invalid range.
    rendererParams.mFrameStart = 0;
    rendererParams.mFrameEnd = -1;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (!arg.empty()) {
            // <nodeName>=<GridAssetUrl>
            std::string urlStr = arg;
            std::string nodeName = "";
            auto        pos = arg.find('=');
            if (pos != std::string::npos) {
                urlStr = arg.substr(pos + 1);
                nodeName = arg.substr(0, pos);
            }

            GridAssetUrl url(urlStr);
            urls.push_back(std::make_pair(nodeName, url));

            // update frame range...
            if (url.isSequence()) {
                if (rendererParams.mFrameEnd < rendererParams.mFrameStart) {
                    rendererParams.mFrameStart = url.frameStart();
                    rendererParams.mFrameEnd = url.frameEnd();
                } else {
                    rendererParams.mFrameStart = nanovdb::Min(rendererParams.mFrameStart, url.frameStart());
                    rendererParams.mFrameEnd = nanovdb::Max(rendererParams.mFrameEnd, url.frameEnd());
                }
            }
        }
    }

    // collect the final parameters...
    // rendererParams.mFrameStart = renderStringParams.get<int>("start", rendererParams.mFrameStart);
    // rendererParams.mFrameEnd = renderStringParams.get<int>("end", rendererParams.mFrameEnd);
    // rendererParams.mWidth = renderStringParams.get<int>("width", rendererParams.mWidth);
    // rendererParams.mHeight = renderStringParams.get<int>("height", rendererParams.mHeight);
    // rendererParams.mOutputFilePath = renderStringParams.get<std::string>("output", rendererParams.mOutputFilePath);
    // rendererParams.mOutputExtension = renderStringParams.get<std::string>("output-format", rendererParams.mOutputExtension);
    // rendererParams.mGoldPrefix = renderStringParams.get<std::string>("gold", rendererParams.mGoldPrefix);
    // rendererParams.mMaterialOverride = renderStringParams.getEnum<MaterialClass>("material-override", kMaterialClassTypeStrings, (int)MaterialClass::kNumTypes, rendererParams.mMaterialOverride);
    // rendererParams.mMaterialBlackbodyTemperature = renderStringParams.get<float>("material-blackbody-temperature", rendererParams.mMaterialBlackbodyTemperature);
    // rendererParams.mMaterialVolumeDensity = renderStringParams.get<float>("material-volume-density", rendererParams.mMaterialVolumeDensity);

    // rendererParams.mSceneParameters.sunDirection = renderStringParams.get<nanovdb::Vec3f>("sun-direction", rendererParams.mSceneParameters.sunDirection);
    // rendererParams.mSceneParameters.samplesPerPixel = renderStringParams.get<int>("camera-samples", rendererParams.mSceneParameters.samplesPerPixel);
    // rendererParams.mSceneParameters.useBackground = renderStringParams.get<bool>("background", rendererParams.mSceneParameters.useBackground);
    // rendererParams.mSceneParameters.useLighting = renderStringParams.get<bool>("lighting", rendererParams.mSceneParameters.useLighting);
    // rendererParams.mSceneParameters.useShadows = renderStringParams.get<bool>("shadows", rendererParams.mSceneParameters.useShadows);
    // rendererParams.mSceneParameters.useGround = renderStringParams.get<bool>("ground", rendererParams.mSceneParameters.useGround);
    // rendererParams.mSceneParameters.useGroundReflections = renderStringParams.get<bool>("ground-reflections", rendererParams.mSceneParameters.useGroundReflections);
    // rendererParams.mSceneParameters.useTonemapping = renderStringParams.get<bool>("tonemapping", rendererParams.mSceneParameters.useTonemapping);
    // rendererParams.mSceneParameters.tonemapWhitePoint = renderStringParams.get<float>("tonemapping-whitepoint", rendererParams.mSceneParameters.tonemapWhitePoint);
    // rendererParams.mSceneParameters.camera.lensType() = renderStringParams.getEnum<Camera::LensType>("camera-lens", kCameraLensTypeStrings, (int)Camera::LensType::kNumTypes, rendererParams.mSceneParameters.camera.lensType());
    // rendererParams.mUseTurntable = renderStringParams.get<bool>("camera-turntable", rendererParams.mUseTurntable);
    // rendererParams.mTurntableRate = renderStringParams.get<float>("camera-turntable-rate", rendererParams.mTurntableRate);
    // rendererParams.mCameraFov = renderStringParams.get<float>("camera-fov", rendererParams.mCameraFov);
    // rendererParams.mCameraDistance = renderStringParams.get<float>("camera-distance", rendererParams.mCameraDistance);
    // rendererParams.mCameraTarget = renderStringParams.get<nanovdb::Vec3f>("camera-target", rendererParams.mCameraTarget);
    // rendererParams.mCameraRotation = renderStringParams.get<nanovdb::Vec3f>("camera-rotation", rendererParams.mCameraRotation);
    // rendererParams.mMaxProgressiveSamples = renderStringParams.get<int>("iterations", rendererParams.mMaxProgressiveSamples);

    // if range still invalid, then make a default frame range...
    if (rendererParams.mFrameEnd < rendererParams.mFrameStart) {
        rendererParams.mFrameStart = 0;
        rendererParams.mFrameEnd = 100;
    }
    try {

        std::unique_ptr<RendererBase> renderer;
        renderer.reset(new Viewer(rendererParams));

        if (platformName.empty() == false) {
            if (renderer->setRenderPlatformByName(platformName) == false) {
                std::cerr << "Unrecognized platform: " << platformName << std::endl;
                return EXIT_FAILURE;
            }
        }

        if (urls.size() > 0) 
        {
            // ensure only one node is made for each specified node name.
            std::map<std::string, std::vector<GridAssetUrl>> nodeGridMap;
            for (auto& nodeUrlPairs : urls) {
                nodeGridMap[nodeUrlPairs.first].push_back(nodeUrlPairs.second);
            }

            // attach the grids.
            for (auto& it : nodeGridMap) {
                if (it.first.length()) {
                    // attach grids to one scene node...
                    int attachmentIndex = 0;
                    auto nodeId = renderer->addSceneNode(it.first);
                    for (size_t i = 0; i < it.second.size(); ++i) {
                        auto& assetUrl = it.second[i];
                        if (assetUrl.scheme() == "file" && assetUrl.gridName().empty()) {
                            auto gridNames = renderer->getGridNamesFromFile(assetUrl);
                            for (auto& gridName : gridNames) {
                                assetUrl.gridName() = gridName;
                                renderer->addGridAsset(assetUrl);
                                renderer->setSceneNodeGridAttachment(nodeId, attachmentIndex++, assetUrl);
                            }
                        } else {
                            renderer->addGridAsset(assetUrl);
                            renderer->setSceneNodeGridAttachment(nodeId, attachmentIndex++, assetUrl);
                        }
                    }
                } else {
                    // create scene nodes for each grid...
                    int nodeIndex = renderer->addGridAssetsAndNodes("default", it.second);
                    if (nodeIndex == -1) {
                        throw std::runtime_error("Some assets have errors.");
                    }
                }
            }

            renderer->selectSceneNodeByIndex(0);
#if 1
            if (auto node = renderer->findNodeByIndex(0)) {
                // waiting for load will enable the frameing to work when we reset the camera!
                if (!renderer->updateNodeAttachmentRequests(node, true, true)) {
                    throw std::runtime_error("Some assets have errors. Unable to render scene node " + node->mName + "; bad asset");
                }
            }
#endif
            renderer->resetCamera();
        }

        renderer->open();
        renderer->run();
        renderer->close();
    }
    catch (const std::exception& e) {
        std::cerr << "An exception occurred: \"" << e.what() << "\"" << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "Exception of unexpected type caught" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
