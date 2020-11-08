
#include <car/car.h>
#include <HardwareSerial.h>

using namespace car;

Car::Car()
{
    constexpr INHPins inhibitPins_{33, 26, 31};
    constexpr PWMPins initPins{10, 22, 23};
    constexpr ISPins isPins{A15, A16, A17};

    constexpr INHPins inhibitPins2{28, 8, 25};
    constexpr PWMPins initPins2{5, 6, 9};
    constexpr ISPins isPins2{A15, A16, A17};
    //    motors.emplace_back(std::move(Motor(inhibitPins_, initPins, 2, isPins)));
    //motors.emplace_back(std::move(Motor(inhibitPins2, initPins2, 2, isPins2)));
    Motor *motor0 = new Motor(inhibitPins_, initPins, 2, isPins);
    Motor *motor1 = new Motor(inhibitPins2, initPins2, 14, isPins2);

    motor_controller = &Controller::getInstance();
    motor_controller->registerMotors(motor0); // 80
    motor0->setAngleOffset(-10);  
    motor0->setAsRightWheel();
    motor_controller->registerMotors(motor1);
    motor0->setAngleOffset(-10); // - 110 is da best for direction, - 10 for the other one                   // - 10 seems aight for CCW

    steering_servo.attach(4); 
}

void Car::uart_init()
{
    Serial.begin(500000); /// init serial
    msg_rx.try_sync();    /// blocks until a sync message arrives
}

void Car::uart_receive()
{
    if (msg_rx.receive())
    { /// check for messages
    using namespace car::com::objects;
        static Object object;
        while (msg_rx.pop_object(object).isValid())
        {
            switch (object.type)
            {
            case TYPE_SYNC:                         /// case sync object
                Time::compute_offset(msg_rx.stamp); /// set clock
                break;
            case TYPE_COMMAND_CAR_ACKERMANN:
            {
                static WheelCommand cmd;
                object.get(cmd);
                motor_controller->setCommand(cmd.wheel[REAR_WHEEL_LEFT ][ROTATION]*100., LEFT);
                motor_controller->setCommand(cmd.wheel[REAR_WHEEL_RIGHT][ROTATION]*100., RIGHT);
                float steering = cmd.wheel[FRONT_WHEEL_LEFT][STEERING]/2. + cmd.wheel[FRONT_WHEEL_RIGHT][STEERING]/2.;
                steering_servo.write(steering*90+90);
            }
            break;
            default: /// case unkown type
                continue;
            }
        }
    }
}

void Car::uart_send()
{
    using namespace car::com::objects;
    msg_tx.reset(); /// removes all objects in message
    if (!text.empty())
    {
        msg_tx.push_object(Object(text, TYPE_TEXT));
    }
    if(false) {
        WheelCommand cmd;
        cmd.wheel[REAR_WHEEL_LEFT ][ROTATION] = motor_controller->getCommand(LEFT) / 100.;
        cmd.wheel[REAR_WHEEL_RIGHT][ROTATION] = motor_controller->getCommand(RIGHT) / 100.;
        cmd.stamp.fromMicros(motor_controller->getTStampCommand());
        msg_tx.push_object(Object(cmd, TYPE_COMMAND_CAR_ACKERMANN));
    }
    if(true){
        RaceCar car_state;
        car_state.wheels[REAR_WHEEL_LEFT].target[ROTATION] = motor_controller->getCommand(LEFT) / 100.;
        car_state.wheels[REAR_WHEEL_RIGHT].target[ROTATION] = motor_controller->getCommand(RIGHT) / 100.;
        car_state.wheels_tstamp.target.fromMicros(motor_controller->getTStampCommand());
        car_state.wheels[REAR_WHEEL_LEFT].speed[ROTATION] = motor_controller->motors[LEFT]->speedRPS;
        car_state.wheels[REAR_WHEEL_RIGHT].speed[ROTATION] = motor_controller->motors[RIGHT]->speedRPS;
        car_state.wheels_tstamp.speed.fromMicros(motor_controller->getTStampMeasurement());
        car_state.stamp = Time::now();
        msg_tx.push_object(Object(car_state, TYPE_RACE_CAR));
    }
    msg_tx.send();
}
Car &Car::getInstance()
{
    static Car instance;
    return instance;
}