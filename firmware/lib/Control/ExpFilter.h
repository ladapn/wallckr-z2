#ifndef EXP_FILTER_H
#define EXP_FILTER_H

#include "IFilter.h"

namespace ExpFilterMath {

template <typename T, int N> static int compute(int input, int state) {
  return static_cast<int>(((N - 1L) * state + input) / N);
}

template <typename T, int N> static float compute(float input, float state) {
  constexpr float input_weight = 1.0f / N;
  constexpr float state_weight = 1.0f - input_weight;

  return state_weight * state + input_weight * input;
}
}; // namespace ExpFilterMath

/**
 * Class implementing exponential filter template
 */
template <typename T, int N> class ExpFilter : public IFilter<T> {
private:
  T m_state;
  bool m_first_step;
  T core(T input) { return ExpFilterMath::compute<T, N>(input, m_state); }

public:
  ExpFilter() : m_state(0), m_first_step(true) {};
  /**
   * Get next filter output value
   * @param[in] input filters input for current step
   * @return filter output value for next step
   */
  T next(T input) override {
    if (m_first_step) {
      m_first_step = false;
      m_state = input;
    } else {
      m_state = core(input);
    }

    return m_state;
  }
};

#endif
