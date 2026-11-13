#ifndef PI_CONTROLLER_H
#define PI_CONTROLLER_H

/**
 * Proportional-integral controller with a clamped output, run at a fixed
 * period.
 *
 * Anti-windup by conditional integration: while the output is clamped, the
 * integral only takes steps that move the output back towards its range, so
 * it doesn't build up during saturation and the output leaves the limit as
 * soon as the error changes sign.
 */
class PiController {
    float m_kp;
    float m_ki_dt;
    float m_output_min;
    float m_output_max;
    // In output units, so the integral term is the sum of ki * e * period.
    float m_integral = 0.0f;

public:
    /**
     * Constructor method
     * @param[in] kp proportional gain, output units per unit of error
     * @param[in] ki integral gain, output units per unit of error and second
     * @param[in] period_s time between calls of update(), in seconds
     * @param[in] output_min lower clamp applied to the output
     * @param[in] output_max upper clamp applied to the output
     */
    PiController(float kp, float ki, float period_s, float output_min, float output_max)
        : m_kp(kp)
        , m_ki_dt(ki * period_s)
        , m_output_min(output_min)
        , m_output_max(output_max)
    {
    }

    /**
     * Compute the next output; call once per period
     * @param[in] setpoint required value of the controlled quantity
     * @param[in] measured actual value of the controlled quantity
     * @return controller output, within [output_min, output_max]
     */
    float update(float setpoint, float measured)
    {
        float e = setpoint - measured;
        float integral = m_integral + m_ki_dt * e;
        float out = m_kp * e + integral;

        if (out > m_output_max) {
            out = m_output_max;
            if (e < 0.0f) {
                m_integral = integral;
            }
        } else if (out < m_output_min) {
            out = m_output_min;
            if (e > 0.0f) {
                m_integral = integral;
            }
        } else {
            m_integral = integral;
        }

        return out;
    }

    /**
     * Forget the integral, e.g. when the controlled system was stopped from
     * outside the loop
     */
    void reset() { m_integral = 0.0f; }
};

#endif // PI_CONTROLLER_H
