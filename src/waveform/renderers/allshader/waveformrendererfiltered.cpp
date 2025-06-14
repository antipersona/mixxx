#include "waveform/renderers/allshader/waveformrendererfiltered.h"

#include "rendergraph/material/rgbmaterial.h"
#include "rendergraph/vertexupdaters/rgbvertexupdater.h"
#include "track/track.h"
#include "util/math.h"
#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveform/waveform.h"

using namespace rendergraph;

namespace allshader {

WaveformRendererFiltered::WaveformRendererFiltered(
        WaveformWidgetRenderer* waveformWidget,
        bool bRgbStacked)
        : WaveformRendererSignalBase(waveformWidget),
          m_bRgbStacked(bRgbStacked) {
    initForRectangles<RGBMaterial>(0);
    setUsePreprocess(true);
}

void WaveformRendererFiltered::onSetup(const QDomNode&) {
}

void WaveformRendererFiltered::preprocess() {
    if (!preprocessInner()) {
        if (geometry().vertexCount() != 0) {
            geometry().allocate(0);
            markDirtyGeometry();
        }
    }
}

bool WaveformRendererFiltered::preprocessInner() {
    TrackPointer pTrack = m_waveformRenderer->getTrackInfo();

    if (!pTrack) {
        return false;
    }

    ConstWaveformPointer waveform = pTrack->getWaveform();
    if (waveform.isNull()) {
        return false;
    }

    const int dataSize = waveform->getDataSize();
    if (dataSize <= 1) {
        return false;
    }

    const WaveformData* data = waveform->data();
    if (data == nullptr) {
        return false;
    }
#ifdef __STEM__
    auto stemInfo = pTrack->getStemInfo();
    // If this track is a stem track, skip the rendering
    if (!stemInfo.isEmpty() && waveform->hasStem()) {
        return false;
    }
#endif

    const float devicePixelRatio = m_waveformRenderer->getDevicePixelRatio();
    const int length = static_cast<int>(m_waveformRenderer->getLength());
    const int pixelLength = static_cast<int>(m_waveformRenderer->getLength() * devicePixelRatio);
    const float invDevicePixelRatio = 1.f / devicePixelRatio;
    const float halfPixelSize = 0.5f / devicePixelRatio;

    // See waveformrenderersimple.cpp for a detailed explanation of the frame and index calculation
    const int visualFramesSize = dataSize / 2;
    const double firstVisualFrame =
            m_waveformRenderer->getFirstDisplayedPosition() * visualFramesSize;
    const double lastVisualFrame =
            m_waveformRenderer->getLastDisplayedPosition() * visualFramesSize;

    // Represents the # of visual frames per horizontal pixel.
    const double visualIncrementPerPixel =
            (lastVisualFrame - firstVisualFrame) / static_cast<double>(pixelLength);

    // Per-band gain from the EQ knobs.
    float allGain(1.0);
    float bandGain[5] = {1.0, 1.0, 1.0, 1.0, 1.0};
//     getGains(&allGain, true, &bandGain[0], &bandGain[1], &bandGain[2]);

    const float breadth = static_cast<float>(m_waveformRenderer->getBreadth());
    const float halfBreadth = breadth / 2.0f;

    const float heightFactor = allGain * halfBreadth / m_maxValue;

    // Effective visual frame for x
    double xVisualFrame = qRound(firstVisualFrame / visualIncrementPerPixel) *
            visualIncrementPerPixel;

    const int numVerticesPerLine = 6; // 2 triangles

    // low, mid, high + horizontal axis
    int reserved = numVerticesPerLine * (pixelLength * m_numBands + 1);

    geometry().setDrawingMode(Geometry::DrawingMode::Triangles);
    geometry().allocate(reserved);
    markDirtyGeometry();

//     QColor lowColor(30,144,255);
//     QColor midColor(255,140,0);
//     QColor highColor(224,255,255);

    QVector3D rgb[m_numBands];
    if (m_bRgbStacked) {
        rgb[0] = QVector3D(static_cast<float>(0),
                static_cast<float>(86/255.f),
                static_cast<float>(1));
        rgb[1] = QVector3D(static_cast<float>(252.f/255.f),
                static_cast<float>(169.f/255.f), // m_rgbMidColor_g
                static_cast<float>(1.f/255.f));
        rgb[2] = QVector3D(static_cast<float>(181.f/255.f), // mid-lows
                static_cast<float>(107.f/255.f),
                static_cast<float>(4.f/255.f));
        rgb[3] = QVector3D(static_cast<float>(246.f/255.f),
                static_cast<float>(235.f/255.f),
                static_cast<float>(215.f/255.f));
        rgb[4] = QVector3D(static_cast<float>(249.f/255.f), // mid-highs
                static_cast<float>(202.f/255.f),
                static_cast<float>(108.f/255.f));
    } else {
        rgb[0] = QVector3D(static_cast<float>(m_lowColor_r),
                static_cast<float>(m_lowColor_g),
                static_cast<float>(m_lowColor_b));
        rgb[1] = QVector3D(static_cast<float>(m_midColor_r),
                static_cast<float>(m_midColor_g),
                static_cast<float>(m_midColor_b));
        rgb[2] = QVector3D(static_cast<float>(m_midColor_r),
                static_cast<float>(m_midColor_g),
                static_cast<float>(m_midColor_b));
        rgb[3] = QVector3D(static_cast<float>(m_highColor_r),
                static_cast<float>(m_highColor_g),
                static_cast<float>(m_highColor_b));
        rgb[4] = QVector3D(static_cast<float>(m_highColor_r),
                static_cast<float>(m_highColor_g),
                static_cast<float>(m_highColor_b));
    }

    RGBVertexUpdater axisVertexUpdater{geometry().vertexDataAs<Geometry::RGBColoredPoint2D>()};
    axisVertexUpdater.addRectangle({0.f,
                                           halfBreadth - 0.5f},
            {static_cast<float>(length),
                    halfBreadth + 0.5f},
            {static_cast<float>(m_axesColor_r),
                    static_cast<float>(m_axesColor_g),
                    static_cast<float>(m_axesColor_b)});

        // reserve space for 5 bands
    RGBVertexUpdater vertexUpdater[m_numBands]{ 
                {geometry().vertexDataAs<Geometry::RGBColoredPoint2D>() +
                        numVerticesPerLine},
                {geometry().vertexDataAs<Geometry::RGBColoredPoint2D>() +
                        numVerticesPerLine * (1 + pixelLength)},
                {geometry().vertexDataAs<Geometry::RGBColoredPoint2D>() +
                        numVerticesPerLine * (1 + pixelLength * 2)},
                {geometry().vertexDataAs<Geometry::RGBColoredPoint2D>() +
                        numVerticesPerLine * (1 + pixelLength * 3)},
                {geometry().vertexDataAs<Geometry::RGBColoredPoint2D>() +
                        numVerticesPerLine * (1 + pixelLength * 4)}
                };

    const double maxSamplingRange = visualIncrementPerPixel / 2.0;

    for (int pos = 0; pos < pixelLength; ++pos) {
        const int visualFrameStart = std::lround(xVisualFrame - maxSamplingRange);
        const int visualFrameStop = std::lround(xVisualFrame + maxSamplingRange);

        const int visualIndexStart = std::max(visualFrameStart * 2, 0);
        const int visualIndexStop =
                std::min(std::max(visualFrameStop, visualFrameStart + 1) * 2, dataSize - 1);

        const float fpos = static_cast<float>(pos) * invDevicePixelRatio;

        // 5 bands, 2 channels
        float max[m_numBands][2]{};
        uchar u8max[3][2]{}; // 3 eqs?
        for (int chn = 0; chn < 2; chn++) {
            for (int i = visualIndexStart + chn; i < visualIndexStop + chn; i += 2) {
                const WaveformData& waveformData = data[i];
                u8max[0][chn] = math_max(u8max[0][chn], waveformData.filtered.low);
                u8max[1][chn] = math_max(u8max[1][chn], waveformData.filtered.mid);
                u8max[2][chn] = math_max(u8max[2][chn], waveformData.filtered.high);
            }
            // Cast to float
            max[0][chn] = static_cast<float>(u8max[0][chn]); // lows    
            max[1][chn] = static_cast<float>(u8max[1][chn]); // mids
            max[2][chn] = static_cast<float>(math_min(u8max[0][chn], u8max[1][chn])); // min between low and mid
            max[3][chn] = static_cast<float>(u8max[2][chn]); // highs
            max[4][chn] = static_cast<float>(math_min(u8max[1][chn], u8max[2][chn])); // highs
        }

        // TODO: this can be optimized by using one geometrynode per band
        // + one for the horizontal axis, and uniform color materials,
        // instead of passing constant color as vertex.

        for (int bandIndex = 0; bandIndex < m_numBands; bandIndex++) {
            max[bandIndex][0] *= bandGain[bandIndex] * allGain;
            max[bandIndex][1] *= bandGain[bandIndex] * allGain;

            vertexUpdater[bandIndex].addRectangle(
                    {fpos - halfPixelSize,
                            halfBreadth - heightFactor * max[bandIndex][0]},
                    {fpos + halfPixelSize,
                            halfBreadth + heightFactor * max[bandIndex][1]},
                    {rgb[bandIndex]});
        }

        xVisualFrame += visualIncrementPerPixel;
    }

    DEBUG_ASSERT(reserved ==
            vertexUpdater[0].index() + vertexUpdater[1].index() +
                    vertexUpdater[2].index());

    markDirtyMaterial();

    return true;
}

} // namespace allshader
