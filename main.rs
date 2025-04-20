use rosrust::{RawMessage};
use std::sync::atomic::{AtomicU64, AtomicBool, Ordering};
use std::sync::Arc;
use rumqttc::{MqttOptions, Client, QoS};
use std::thread;
use std::time::Duration;

struct Adapter {
    name: String,
    topic: String,
    mqtt_topic: String,
    variable: Arc<AtomicU64>,
    updated: Arc<AtomicBool>,
    retrival_function: Arc<dyn Fn(RawMessage) -> u64 + Send + Sync>,
    float_flag: bool,
}

fn main() {

    macro_rules! subscribe_to_rostopic {
        ($adapter: expr) => {{
            let updated_clone = $adapter.updated.clone();
            let variable_clone = $adapter.variable.clone();
            let retrival_function = $adapter.retrival_function.clone();
            rosrust::subscribe(
                &$adapter.topic,
                100, move |v: RawMessage| {
                    let value = retrival_function(v);
                    if variable_clone.load(Ordering::Relaxed) != value {
                        variable_clone.store(value,Ordering::Relaxed);
                        updated_clone.store(true,Ordering::Relaxed);
                    }
                }
                ).unwrap()
        }};
    }

    macro_rules! publish_to_mqtt {
        ($topic: expr, $payload: expr, $client: expr) => {
            //println!("{}",$payload);
            $client.publish($topic, QoS::AtLeastOnce, false, $payload.as_bytes()).unwrap();
        }
    }
    
    // Setup Adapters
    let mut adapters = vec!();

    let linear_x_adapter = Adapter {
        name:String::from("linear_x"),
        topic:String::from("sam/cmd_vel"),
        mqtt_topic:String::from("/robot/control/cmd_vel/linear_x"),
        variable:Arc::new(AtomicU64::new(0)),
        updated:Arc::new(AtomicBool::new(true)),
        retrival_function:
            Arc::new(|i:RawMessage| -> u64 {f64::from_le_bytes(i.0[0..8].try_into().unwrap()).to_bits()}),
            float_flag:true,
    };

    let angular_z_adapter = Adapter {
        name:String::from("angular_z"),
        topic:String::from("sam/cmd_vel"),
        mqtt_topic:String::from("/robot/control/cmd_vel/angular_z"),
        variable:Arc::new(AtomicU64::new(0)),
        updated:Arc::new(AtomicBool::new(true)),
        retrival_function:
            Arc::new(|i:RawMessage| -> u64 {f64::from_le_bytes(i.0[40..48].try_into().unwrap()).to_bits()}),
            float_flag:true,
    };

    let battery_percentage_adapter = Adapter {
        name:String::from("battery_percentage"),
        topic:String::from("sam/battery_percentage"),
        mqtt_topic:String::from("/robot/state/battery_percentage"),
        variable:Arc::new(AtomicU64::new(0)),
        updated:Arc::new(AtomicBool::new(true)),
        retrival_function:
            Arc::new(|i:RawMessage| -> u64 {f64::from_le_bytes(i.0.try_into().unwrap()).to_bits()}),
            float_flag:true,
    };

    let battery_is_charging_adapter = Adapter {
        name:String::from("battery_is_charging"),
        mqtt_topic:String::from("/robot/state/battery_is_charging"),
        topic:String::from("sam/battery_is_charging"),
        variable:Arc::new(AtomicU64::new(0)),
        updated:Arc::new(AtomicBool::new(true)),
        retrival_function:
            Arc::new(|i:RawMessage| -> u64 {(i.0)[0] as u64}),
            float_flag:false,
    };

    let e_stop_adapter = Adapter {
        name:String::from("e_stop"),
        topic:String::from("/emergencystate"),
        mqtt_topic:String::from("/robot/state/e_stop"),
        variable:Arc::new(AtomicU64::new(0)),
        updated:Arc::new(AtomicBool::new(true)),
        retrival_function:
            Arc::new(|i:RawMessage| -> u64 {(i.0)[0] as u64}),
            float_flag:false,
    };

    let handbrake_adapter = Adapter {
        name:String::from("handbrake"),
        topic:String::from("/i2r_system_monitor/status"),
        mqtt_topic:String::from("/robot/state/handbrake"),
        variable:Arc::new(AtomicU64::new(0)),
        updated:Arc::new(AtomicBool::new(true)),
        retrival_function:
            Arc::new(|i:RawMessage| -> u64 {
                let len = u32::from_le_bytes([(i.0)[0], (i.0)[1], (i.0)[2], (i.0)[3]]) as usize;
                let extr_string = String::from_utf8_lossy(&(i.0)[4..(4 + len)]);
                let parsed_msg = json::parse(&extr_string).unwrap();
                let extracted = &parsed_msg["payload"]["data"][1]["system_status_data"]["data"]["system_status"];
                format!("{:#}",extracted["handbrake_status"]).parse::<u64>().unwrap()
            }),
            float_flag:false,
    };

    let direct_status_adapter = Adapter {
        name:String::from("direct_status"),
        topic:String::from("/i2r_system_monitor/status"),
        mqtt_topic:String::from("/robot/state/direct_status"),
        variable:Arc::new(AtomicU64::new(0)),
        updated:Arc::new(AtomicBool::new(true)),
        retrival_function:
            Arc::new(|i:RawMessage| -> u64 {
                let len = u32::from_le_bytes([(i.0)[0], (i.0)[1], (i.0)[2], (i.0)[3]]) as usize;
                let extr_string = String::from_utf8_lossy(&(i.0)[4..(4 + len)]);
                let parsed_msg = json::parse(&extr_string).unwrap();
                let extracted = &parsed_msg["payload"]["data"][1]["system_status_data"]["data"]["system_status"];
                format!("{:#}",extracted["direct_status"]).parse::<u64>().unwrap()
            }),
            float_flag:false,
    };

    let robot_mode_adapter = Adapter {
        name:String::from("robot_mode"),
        topic:String::from("/robot_controller/robot_mode"),
        mqtt_topic:String::from("/robot/state/robot_mode"),
        variable:Arc::new(AtomicU64::new(0)),
        updated:Arc::new(AtomicBool::new(true)),
        retrival_function:
            Arc::new(|i:RawMessage| -> u64 {u32::from_le_bytes(i.0.try_into().expect("")) as u64}),
            float_flag:false,
    };

    let brush_speed_adapter = Adapter {
        name:String::from("brush_speed"),
        topic:String::from("/sidebrush_controller/sidebrush_speed_feedback"),
        mqtt_topic:String::from("/robot/control/brush_speed"),
        variable:Arc::new(AtomicU64::new(0)),
        updated:Arc::new(AtomicBool::new(true)),
        retrival_function:
            Arc::new(|i:RawMessage| -> u64 {
                u32::from_le_bytes(i.0[..4].try_into().expect("")) as u64
            }),
            float_flag:false,
    };

    let safety_mode_adapter = Adapter {
        name:String::from("safety_mode"),
        topic:String::from("/i2r_system_monitor/status"),
        mqtt_topic:String::from("/robot/state/safety_mode"),
        variable:Arc::new(AtomicU64::new(0)), 
        updated:Arc::new(AtomicBool::new(true)),
        retrival_function:
            Arc::new(|i:RawMessage| -> u64 {
                let len = u32::from_le_bytes([(i.0)[0], (i.0)[1], (i.0)[2], (i.0)[3]]) as usize; 
                let extr_string = String::from_utf8_lossy(&(i.0)[4..(4 + len)]); 
                let parsed_msg = json::parse(&extr_string).unwrap();
                let extracted = &parsed_msg["payload"]["data"][1]["system_status_data"]["data"]["motor_filter"];
                format!("{:#}",extracted["safety_mode"]).parse::<u64>().unwrap()
            }), 
        float_flag:false,
    };

    // Setup MQTT and ROS
    let mut mqttoptions = MqttOptions::new("dv8_rpii_mqtt", "192.168.0.108", 1883);
    mqttoptions.set_keep_alive(Duration::from_secs(5));
    let (client, mut connection) = Client::new(mqttoptions, 10);
    //rosrust::init("dv8_rpi_mqtt");

    loop {
        match rosrust::try_init("dv8_rpi_mqtt") {
            Ok(_) => {
                println!("Connected to ROS master!");
                break;
            }
            Err(e) => {
                eprintln!("Failed to connect to ROS master: {}. Retrying...", e);
                thread::sleep(Duration::from_millis(1000));
            }
        }
    }

    // Initializing services
    let robot_mode_client = rosrust::client::<rosrust_msg::bumperbot_controller::GetCurrentMode>("robot_controller/get_current_mode").unwrap();
    let robot_mode_initial = robot_mode_client
        .req(&rosrust_msg::bumperbot_controller::GetCurrentModeReq{})
        .unwrap()
        .unwrap()  
        .mode;
    robot_mode_adapter.variable.store(robot_mode_initial as u64, Ordering::Relaxed);

    // Add Subscribers
    let _subscriber_1 = subscribe_to_rostopic!(linear_x_adapter);
    adapters.push(linear_x_adapter);
    let _subscriber_2 = subscribe_to_rostopic!(angular_z_adapter);
    adapters.push(angular_z_adapter);
    let _subscriber_3 = subscribe_to_rostopic!(battery_is_charging_adapter);
    adapters.push(battery_is_charging_adapter);    
    let _subscriber_4 = subscribe_to_rostopic!(e_stop_adapter);
    adapters.push(e_stop_adapter);
    let _subscriber_5 = subscribe_to_rostopic!(handbrake_adapter);
    adapters.push(handbrake_adapter);
    let _subscriber_6 = subscribe_to_rostopic!(direct_status_adapter);
    adapters.push(direct_status_adapter);
    let _subscriber_7 = subscribe_to_rostopic!(robot_mode_adapter);
    adapters.push(robot_mode_adapter);
    let _subscriber_8 = subscribe_to_rostopic!(brush_speed_adapter);
    adapters.push(brush_speed_adapter);
    let _subscriber_9 = subscribe_to_rostopic!(battery_percentage_adapter);
    adapters.push(battery_percentage_adapter);
    let _subscriber_10 = subscribe_to_rostopic!(safety_mode_adapter);
    adapters.push(safety_mode_adapter);

    //Do not remove
    thread::spawn(move || {
        for (_i,_notification) in connection.iter().enumerate() {
            //println!("Notification = {:?}",notification);
        }
    });

    // Publisher loop
    while rosrust::is_ok() {
        let mut updates = 0;
        for adapter in &adapters {
            if adapter.updated.load(Ordering::Relaxed) {
                updates = updates + 1;
            }
        }
        if updates == 0 {
            thread::sleep(Duration::from_millis(500));
        }
        for adapter in &adapters {
            let variable = adapter.variable.load(Ordering::Relaxed);
            let payload;
            if adapter.float_flag {
                payload = format!(r#"{{"{}":{}}}"#,&adapter.name,f64::from_bits(variable));
            } else {
                payload = format!(r#"{{"{}":{}}}"#,&adapter.name,variable);
            }
            publish_to_mqtt!(adapter.mqtt_topic.clone(),payload,client);
            adapter.updated.store(false, Ordering::Relaxed);
        }
    }
    rosrust::spin();
}

