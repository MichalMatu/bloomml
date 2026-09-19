#include <cstdio>
#include <cstring>

#include "growbox_clay_ui/HostSimulator.h"

int main(int argc, char** argv) {
  bool selfCheck = false;
  const char* outputPath = nullptr;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--self-check") == 0) {
      selfCheck = true;
    } else if (std::strcmp(argv[i], "--pbm") == 0 && i + 1 < argc) {
      outputPath = argv[++i];
    } else {
      std::fprintf(stderr, "usage: %s [--self-check] [--pbm PATH]\n", argv[0]);
      return 2;
    }
  }

  growbox::clay_ui::MonochromeFrame frame;
  growbox::clay_ui::RenderSummary summary;
  if (!growbox::clay_ui::renderHostSmoke(frame, &summary)) {
    std::fprintf(stderr, "Clay host render failed\n");
    return 1;
  }
  if (summary.width != 296 || summary.height != 128 || frame.bytes.size() != 4736U ||
      summary.black_pixels == 0 || summary.render_commands == 0) {
    std::fprintf(stderr, "Clay host geometry/self-check failed\n");
    return 1;
  }
  if (outputPath && !growbox::clay_ui::writePbm(frame, outputPath)) {
    std::fprintf(stderr, "failed to write PBM: %s\n", outputPath);
    return 1;
  }
  if (!selfCheck) {
    std::printf("growbox Clay host simulator: %ux%u, bytes=%zu, commands=%zu, black=%zu\n",
                static_cast<unsigned>(summary.width), static_cast<unsigned>(summary.height),
                frame.bytes.size(), summary.render_commands, summary.black_pixels);
  }
  return 0;
}
