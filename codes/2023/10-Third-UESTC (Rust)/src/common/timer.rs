use std::thread::sleep;
use std::time::{Duration, SystemTime};

pub struct Timer {
    start: SystemTime,
}

impl Timer {
    pub fn new() -> Timer {
        Timer {
            start: SystemTime::now(),
        }
    }

    pub fn cost(&self) -> f32 {
        SystemTime::now()
            .duration_since(self.start)
            .unwrap()
            .as_secs_f32()
    }

    pub fn sleep(secs: u64) {
        sleep(Duration::from_secs(secs));
    }

    pub fn reset(&mut self) {
        self.start = SystemTime::now()
    }
}
