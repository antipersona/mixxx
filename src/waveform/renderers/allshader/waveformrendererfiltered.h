#pragma once

#include "rendergraph/geometrynode.h"
#include "util/class.h"
#include "waveform/renderers/allshader/waveformrenderersignalbase.h"

namespace allshader {
class WaveformRendererFiltered;
} // namespace allshader

class allshader::WaveformRendererFiltered final
        : public allshader::WaveformRendererSignalBase,
          public rendergraph::GeometryNode {
  public:
    explicit WaveformRendererFiltered(WaveformWidgetRenderer* waveformWidget,
            bool rgbStacked);

    // Pure virtual from WaveformRendererSignalBase, not used
    void onSetup(const QDomNode& node) override;

    // Virtuals for rendergraph::Node
    void preprocess() override;

  private:
    const bool m_bRgbStacked;
    bool preprocessInner();
    constexpr static int m_numBands = 4;
    float m_allGain = .85; // fix temporal porque bandGain no parece funcionar

    DISALLOW_COPY_AND_ASSIGN(WaveformRendererFiltered);
};
