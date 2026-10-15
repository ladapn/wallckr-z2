#ifndef IFILTER_H
#define IFILTER_H

/**
 * Interface for data filtering classes
 */
template <typename T> class IFilter {
public:
  virtual ~IFilter() = default;

  /**
   * Get next filter output value
   * @param[in] input filters input for current step
   * @return filter output value for next step
   */
  virtual T next(T input) = 0;
};

#endif
