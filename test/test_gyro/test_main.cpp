#include <unity.h>
// =============================================================================
// Simulate the core logic of processing raw gyro data in a native C++
// environment using Unity Test Framework. This allows us to test the conversion
// and filtering algorithms without needing the actual hardware.
// =============================================================================

// Function 1: Convert raw gyro data (int16_t) to degrees per second (float)
// With the MPU-6050, the sensitivity is 131 LSB/(°/s), so we divide the raw
// value by 131.
float convertRawGyroToDps(int16_t rawValue) { return (float)rawValue / 131.0; }

// Function 2: Filter noise (Deadband). If the circuit is at rest but
// experiencing slight vibration below the threshold (e.g., 1.5 °/s), we set it
// to 0 to prevent drift.
float applyDeadband(float dpsValue, float threshold) {
  if (dpsValue > -threshold && dpsValue < threshold) {
    return 0.0;
  }
  return dpsValue;
}

void setUp(void) {}
void tearDown(void) {}

// =========================================================================
// Test cases for the gyro processing functions
// =========================================================================

// Case 1: Test positive rotation (Right/Up)
void test_gyro_positive_rotation(void) {
  int16_t raw = 13100;  // Rotate approximately 100 °/s
  float expected = 100.0;

  TEST_ASSERT_FLOAT_WITHIN(0.01, expected, convertRawGyroToDps(raw));
}

// Case 2: Test negative rotation (Left/Down)
void test_gyro_negative_rotation(void) {
  int16_t raw = -6550;  // Rotate approximately -50 °/s
  float expected = -50.0;
  TEST_ASSERT_FLOAT_WITHIN(0.01, expected, convertRawGyroToDps(raw));
}

// Case 3: Test circuit at rest but experiencing slight vibration
void test_gyro_noise_filtering(void) {
  float noisyValue = 1.2;
  float threshold = 1.5;

  float filtered = applyDeadband(noisyValue, threshold);

  TEST_ASSERT_EQUAL_FLOAT(0.0, filtered);
}

// Case 4: Test circuit actually moving (exceeding noise threshold)
void test_gyro_real_movement(void) {
  float realMovement = 45.5;  // Rotate 45.5 °/s
  float threshold = 1.5;

  float filtered = applyDeadband(realMovement, threshold);

  // Because 45.5 > 1.5, the filter must retain the original value
  TEST_ASSERT_EQUAL_FLOAT(45.5, filtered);
}

// =========================================================================
// Main function to run the tests. Unity will call each test function and report
// the results.
// =========================================================================
int main(int argc, char** argv) {
  UNITY_BEGIN();

  RUN_TEST(test_gyro_positive_rotation);
  RUN_TEST(test_gyro_negative_rotation);

  RUN_TEST(test_gyro_noise_filtering);
  RUN_TEST(test_gyro_real_movement);

  return UNITY_END();
}