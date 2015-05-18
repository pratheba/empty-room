#ifndef _SOLVER_H
#define _SOLVER_H
#include <vector>

#include "R3Shapes/R3Shapes.h"
#include "datamanager/imagemanager.h"
#include "datamanager/meshmanager.h"
#include "hemicuberenderer.h"

class InverseRender {
    public:
        InverseRender(MeshManager* m, int nlights, int hemicubeResolution=150)
            : mesh(m), numlights(nlights), images(NULL),
              maxPercentErr(0.1), numRansacIters(1000)
        {
            rm = new RenderManager(m);
            rm->setupMeshColors();
            hr = new HemicubeRenderer(rm, hemicubeResolution);
            lights.resize(numlights);
        }
        ~InverseRender() {
            if (hr) delete hr;
            if (rm) delete rm;
        }
        void solve(std::vector<SampleData>& data);
        void solveTexture(
                std::vector<SampleData>& data,
                ImageManager* imagemanager,
                const R3Plane& surface,
                Texture& tex);
        void computeSamples(
                std::vector<SampleData>& data,
                std::vector<int> indices,
                int numsamples,
                double discardthreshold=1,
                bool saveImages=true);

        void writeVariablesMatlab(std::vector<SampleData>& data, std::string filename);
        void writeVariablesBinary(std::vector<SampleData>& data, std::string filename);
        void loadVariablesBinary(std::vector<SampleData>& data, std::string filename);

        void setNumRansacIters(int n) { numRansacIters = n; }
        void setMaxPercentErr(double d) { maxPercentErr = d; }
        RenderManager* getRenderManager() { return rm; }
    private:
        // Inverse Rendering helpers
        bool calculateWallMaterialFromUnlit(std::vector<SampleData>& data);
        bool solveAll(std::vector<SampleData>& data);

        // Texture recovery helpers
        Material computeAverageMaterial(
                std::vector<SampleData>& data,
                std::vector<Material>& lightintensies);
        double generateBinaryMask(const CameraParams* cam, const char* labelimage, std::vector<bool>& mask, int label);

        HemicubeRenderer* hr;
        RenderManager* rm;

        int numRansacIters;
        double maxPercentErr;
    public:
        float** images;
        MeshManager* mesh;
        int numlights;
        std::vector<Material> lights;
        Material wallMaterial;
};

#endif
