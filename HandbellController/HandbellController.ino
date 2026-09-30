#include "HandbellController.h"
#include "Joystick.h"
#include "KeyboardMode.h"
#include "MPU6050YPRAccelerometer.h"
#include <EEPROM.h>
#include <Wire.h>

#ifdef ENABLE_LIS3DH
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#endif

enum
{
    ADDR_18_LIS3DH = 0x18,
    ADDR_19_LIS3DH = 0x19,
    ADDR_1D_ADXL345 = 0x1D,
    ADDR_53_ADXL345 = 0x53,
    ADDR_68_MPU9250 = 0x68,
    ADDR_69_MPU9250 = 0x69,
};

////////////////////////////////////////////////////////////////////
// LIS3DH
////////////////////////////////////////////////////////////////////
#ifdef ENABLE_LIS3DH
class LIS3DHAccelerometer : public Accelerometer
{
private:
    int _address;
    Adafruit_LIS3DH _lis = Adafruit_LIS3DH();

public:
    LIS3DHAccelerometer(int address)
    {
        _address = address;
    }

    accel_t GetType() const override { return ACCEL_LIS3DH; }

    void Setup(Joystick* joystick) override
    {
        _lis.begin(_address);
        _lis.setRange(LIS3DH_RANGE_2_G);
        _lis.setDataRate(LIS3DH_DATARATE_50_HZ);
        joystick->setXAxisRange(-16384, 16383);
        joystick->setYAxisRange(-16384, 16383);
        joystick->setZAxisRange(-16384, 16383);
    }

    void Update(Joystick* joystick) override
    {
        _lis.read();
        joystick->setXAxis(_lis.x);
        joystick->setYAxis(_lis.y);
        joystick->setZAxis(_lis.z);
    }
};
#endif

////////////////////////////////////////////////////////////////////
// MPU6050
////////////////////////////////////////////////////////////////////
#ifdef ENABLE_MPU6050
#define MPU6050_RA_ACCEL_XOUT_H 0x3B
#define MPU6050_RA_ACCEL_XOUT_L 0x3C
#define MPU6050_RA_ACCEL_YOUT_H 0x3D
#define MPU6050_RA_ACCEL_YOUT_L 0x3E
#define MPU6050_RA_ACCEL_ZOUT_H 0x3F
#define MPU6050_RA_ACCEL_ZOUT_L 0x40
#define MPU6050_RA_GYRO_XOUT_H 0x43
#define MPU6050_RA_GYRO_XOUT_L 0x44
#define MPU6050_RA_GYRO_YOUT_H 0x45
#define MPU6050_RA_GYRO_YOUT_L 0x46
#define MPU6050_RA_GYRO_ZOUT_H 0x47
#define MPU6050_RA_GYRO_ZOUT_L 0x48
#define MPU6050_RA_PWR_MGMT_1 0x6B

class MPU6050Accelerometer : public Accelerometer
{
private:
    int _address;

public:
    MPU6050Accelerometer(int address)
    {
        _address = address;
    }

    accel_t GetType() const override { return ACCEL_MPU6050; }

    void Setup(Joystick* joystick) override
    {
        Wire.beginTransmission(_address);
        Wire.write(MPU6050_RA_PWR_MGMT_1); // Talk to the register 6B
        Wire.write(0x00);                  // Make reset - place a 0 into the 6B register
        Wire.endTransmission();
        joystick->setXAxisRange(-16384, 16383);
        joystick->setYAxisRange(-16384, 16383);
        joystick->setZAxisRange(-16384, 16383);
        joystick->setRxAxisRange(-16384, 16383);
        joystick->setRyAxisRange(-16384, 16383);
        joystick->setRzAxisRange(-16384, 16383);
    }

    void Update(Joystick* joystick) override
    {
        // Read acceleratometer data
        Wire.beginTransmission(_address);
        Wire.write(MPU6050_RA_ACCEL_XOUT_H);
        Wire.endTransmission();
        Wire.requestFrom(_address, 6, true);
        float accX = (Wire.read() << 8 | Wire.read());
        float accY = (Wire.read() << 8 | Wire.read());
        float accZ = (Wire.read() << 8 | Wire.read());

        // Read gyroscope data
        Wire.beginTransmission(_address);
        Wire.write(MPU6050_RA_GYRO_XOUT_H);
        Wire.endTransmission();
        Wire.requestFrom(_address, 6, true);
        float gyroX = (Wire.read() << 8 | Wire.read());
        float gyroY = (Wire.read() << 8 | Wire.read());
        float gyroZ = (Wire.read() << 8 | Wire.read());

        // Set joystick state
        joystick->setXAxis(accX);
        joystick->setYAxis(accY);
        joystick->setZAxis(accZ);
        joystick->setRxAxis(gyroX);
        joystick->setRyAxis(gyroY);
        joystick->setRzAxis(gyroZ);
    }
};
#endif

////////////////////////////////////////////////////////////////////
// ADXL345
////////////////////////////////////////////////////////////////////
#ifdef ENABLE_ADXL345

#define ADXL345_POWER_CTL 0x2D
#define ADXL345_MEASUREMENT_RANGE 0x31
#define ADXL345_ACCEL_XOUT_H 0x32

class ADXL345Accelerometer : public Accelerometer
{
private:
    int _address;

public:
    ADXL345Accelerometer(int address)
    {
        _address = address;
    }

    accel_t GetType() const override { return ACCEL_ADXL345; }

    void Setup(Joystick* joystick, const SavedConfig* config) override
    {
        Wire.beginTransmission(_address);
        Wire.write(ADXL345_POWER_CTL); // Access / talk to POWER_CTL Register - 0x2D
        Wire.write(8);                 // Enable measurement (8dec -> 0000 1000 binary) Bit D3 High for measuring enable
        Wire.endTransmission();

        Wire.beginTransmission(_address);
        Wire.write(ADXL345_MEASUREMENT_RANGE); // Set the measurement range
        Wire.write(0);                         // Set 10-bit mode with range -2g..+2g
        Wire.endTransmission();

        joystick->setXAxisRange(-512, 511);
        joystick->setYAxisRange(-512, 511);
        joystick->setZAxisRange(-512, 511);
    }

    void Update(Joystick* joystick) override
    {
        Wire.beginTransmission(_address);
        Wire.write(ADXL345_ACCEL_XOUT_H);
        Wire.endTransmission();
        Wire.requestFrom(_address, 6, true);
        float accX = (Wire.read() | Wire.read() << 8);
        float accY = (Wire.read() | Wire.read() << 8);
        float accZ = (Wire.read() | Wire.read() << 8);

        joystick->setXAxis(accX);
        joystick->setYAxis(accY);
        joystick->setZAxis(accZ);
    }
};
#endif

static Accelerometer* CreateAccelerometer()
{
    // Try every I2C address to see which one responds.
    // Addresses 0..7 and 120..127 are reserved.
    // We are assuming that only an accelerometer is connected.
    for (int address = 8; address < 120; address++)
    {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0)
        {
            switch (address)
            {
#ifdef ENABLE_LIS3DH
                case ADDR_18_LIS3DH:
                case ADDR_19_LIS3DH:
                    return new LIS3DHAccelerometer(address);
#endif
#ifdef ENABLE_ADXL345
                case ADDR_1D_ADXL345:
                case ADDR_53_ADXL345:
                    return new ADXL345Accelerometer(address);
#endif
#if defined(ENABLE_MPU6050)
                case ADDR_68_MPU9250:
                case ADDR_69_MPU9250:
                    return new MPU6050Accelerometer(address);
#elif defined(ENABLE_MPU6050_YPR)
                case ADDR_68_MPU9250:
                case ADDR_69_MPU9250:
                    return new MPU6050YPRAccelerometer(address);
#endif
            }
        }
    }
    return nullptr;
}

constexpr int LED_PIN = 17;
constexpr int CALIBRATION_HOLD_TIME = 20 * 1000; // 20 seconds
constexpr char* EEPROM_MAGIC = "eBells  ";

static Joystick* gJoystick;
static Accelerometer* gAccelerometer;

static uint32_t _lastTime = 0;
static uint32_t _lightState = 0;
static int16_t _lastX = 0;
static int16_t _lastY = 0;
static int16_t _lastZ = 0;
static bool _blinkOff = false;
static bool _buttonState[2];
static uint32_t _calibrationHoldTime = 0;
static bool _calibrationMode = false;
static SavedConfig _config;

static void SetupButtons()
{
    pinMode(9, INPUT_PULLUP);
    pinMode(10, INPUT_PULLUP);
}

static void UpdateButtons(Joystick* joystick)
{
    _buttonState[0] = !digitalRead(10);
    _buttonState[1] = !digitalRead(9);

#ifdef SWAP_BUTTONS
    // Swap buttons around for any e-bell that has them the wrong way round
    joystick->setButton(0, _buttonState[1]);
    joystick->setButton(1, _buttonState[0]);
#else
    joystick->setButton(0, _buttonState[0]);
    joystick->setButton(1, _buttonState[1]);
#endif
}

static void SetupLights()
{
    pinMode(LED_PIN, OUTPUT);
}

static void UpdateLights(Joystick* joystick)
{
    auto currentTime = millis();
    if (currentTime > _lastTime + 500)
    {
        _lastTime = currentTime;

        auto showLight = false;
        if (_blinkOff)
        {
            _blinkOff = false;
        }
        else
        {
            switch (_lightState)
            {
                case 0:
                    showLight = joystick->getButton(0);
                    break;
                case 1:
                    showLight = joystick->getButton(1);
                    break;
                case 2:
                    showLight = joystick->getXAxis() != _lastX;
                    _lastX = joystick->getXAxis();
                    break;
                case 3:
                    showLight = joystick->getYAxis() != _lastY;
                    _lastY = joystick->getYAxis();
                    break;
                case 4:
                    showLight = joystick->getZAxis() != _lastZ;
                    _lastZ = joystick->getZAxis();
                    break;
                case 5:
                case 6:
                    // Restart delay
                    break;
                default:
                    _lightState = -1;
                    break;
            }
            _lightState++;
            _blinkOff = true;
        }

        if (showLight)
            digitalWrite(LED_PIN, 0);
        else
            digitalWrite(LED_PIN, 1);
    }
}

static void SaveCalibration()
{
    strncpy(_config.magic, EEPROM_MAGIC, 8);
    EEPROM.put(0, _config);
}

static void LoadCalibration()
{
    EEPROM.get(0, _config);
    if (memcmp(_config.magic, EEPROM_MAGIC, 8) != 0)
    {
        memset(&_config, 0, sizeof(_config));
        SaveCalibration();
    }
}

static void FlashLight(int count)
{
    for (int i = 0; i < count; i++)
    {
        digitalWrite(LED_PIN, 0);
        delay(150);
        digitalWrite(LED_PIN, 1);
        delay(250);
    }
}

// Hold a cap button while plugging in to choose keyboard output: button 0
// alone for a right-hand bell (J), button 1 alone for a left-hand bell (F),
// or both to turn it off. The LED flashes 1, 2 or 3 times to confirm, and
// the choice is saved.
static void ChooseKeyMode()
{
    auto readButtons = []() {
#ifdef SWAP_BUTTONS
        return (uint8_t)((!digitalRead(9) ? 1 : 0) | (!digitalRead(10) ? 2 : 0));
#else
        return (uint8_t)((!digitalRead(10) ? 1 : 0) | (!digitalRead(9) ? 2 : 0));
#endif
    };

    auto held = readButtons();
    if (held == 0)
        return;
    delay(500);
    if (readButtons() != held)
        return;

    uint8_t mode = held == 1 ? KEYMODE_RIGHT : held == 2 ? KEYMODE_LEFT : KEYMODE_OFF;
    if (_config.keyMode != mode)
    {
        _config.keyMode = mode;
        SaveCalibration();
    }
    FlashLight(mode == KEYMODE_RIGHT ? 1 : mode == KEYMODE_LEFT ? 2 : 3);
}

static bool UpdateCalibrationMode()
{
    if (_calibrationMode)
    {
        if (!calibration_loop())
        {
            calibration_get(&_config.acc, &_config.gyro);
            SaveCalibration();
            _calibrationMode = false;
        }
    }
    else
    {
        // Check if both buttons are held down long enough to enter calibration mode
        auto currentTime = millis();
        if (_buttonState[0] && _buttonState[1])
        {
            if (currentTime > _calibrationHoldTime + CALIBRATION_HOLD_TIME)
            {
                _calibrationMode = true;
                calibration_setup();
            }
        }
        else
        {
            _calibrationHoldTime = currentTime;
        }
    }
    return _calibrationMode;
}

void setup()
{
    Wire.begin();

    LoadCalibration();
    SetupButtons();
    SetupLights();
    ChooseKeyMode();
    keymode_setup(_config.keyMode);

    gAccelerometer = CreateAccelerometer();
    if (gAccelerometer != nullptr)
    {
        bool hasGyro = gAccelerometer->GetType() == ACCEL_MPU6050;
#ifdef ENABLE_MPU6050_YPR
        bool hasYPR = false;
#else
        bool hasYPR = false;
#endif
        gJoystick = new Joystick(
            JOYSTICK_DEFAULT_REPORT_ID,
            JOYSTICK_TYPE_JOYSTICK,
            2,
            0,
            true,
            true,
            true,
            hasGyro,
            hasGyro,
            hasGyro,
            hasYPR,
            hasYPR,
            hasYPR,
            false,
            false);
        gAccelerometer->Setup(gJoystick, &_config);
    }
    else
    {
        gJoystick = new Joystick(
            JOYSTICK_DEFAULT_REPORT_ID,
            JOYSTICK_TYPE_JOYSTICK,
            2,
            0,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false);
    }

    gJoystick->begin(false);

    delay(10);
}

void loop()
{
    if (!UpdateCalibrationMode())
    {
        if (gAccelerometer != nullptr)
        {
            gAccelerometer->Update(gJoystick);
        }

        UpdateButtons(gJoystick);
        keymode_update(gJoystick->getZAxis(), gJoystick->getButton(0), gJoystick->getButton(1));
        UpdateLights(gJoystick);
        gJoystick->sendState();
    }
}
