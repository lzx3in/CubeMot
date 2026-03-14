#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int button_detector_init(void);

#ifdef BUILD_TESTING
void button_detector_deinit(void);
#endif

#ifdef __cplusplus
}
#endif
