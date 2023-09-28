#include <nanovdb/util/IO.h> // this is required to read (and write) NanoVDB files on the host
#include <iomanip>
#include <numeric>
#include "StringUtils.h"
#include "Renderer.h"
#include "Viewer.h"

int main(int argc, char* argv[])
{
    std::string                                       platformName;
    std::vector<std::pair<std::string, GridAssetUrl>> urls;
    RendererParams                                    rendererParams;

    bool batch = false;

    rendererParams.mWidth = 1024;
    rendererParams.mHeight = 1024;
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
