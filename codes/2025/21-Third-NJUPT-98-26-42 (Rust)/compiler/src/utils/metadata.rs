//! 编译元数据记录

use crate::prelude::*;
use std::{collections::HashMap, io::Write, time::Instant};
use strum::IntoEnumIterator;

/// 编译元数据记录
#[derive(Debug, Default)]
pub struct CompilerMetadata {
    pub start_time: Option<Instant>,
    pub phase_timings: HashMap<CompilerStage, std::time::Duration>,
    pub total_duration: Option<std::time::Duration>,
}

impl CompilerMetadata {
    pub fn new() -> Self {
        Self {
            start_time: Some(Instant::now()),
            ..Default::default()
        }
    }

    pub fn phase_start(&mut self, _phase: CompilerStage) -> Instant {
        Instant::now()
    }

    pub fn phase_end(&mut self, phase: CompilerStage, start: Instant) {
        self.phase_timings.insert(phase, start.elapsed());
    }

    pub fn finalize(&mut self) {
        if let Some(start) = self.start_time {
            self.total_duration = Some(start.elapsed());
        }
    }

    /// 输出带颜色的计时信息
    pub fn timing_line(&self) -> String {
        let total_ms = self.total_duration.map(|d| d.as_millis()).unwrap_or(0);
        let phase_parts: Vec<String> = CompilerStage::iter()
            .filter_map(|stage| {
                self.phase_timings.get(&stage).map(|duration| {
                    let ms = duration.as_millis();
                    // 大于 5s 就认为有大问题
                    let color = if ms > 5000 { "\\e[31m" } else { "\\e[90m" };
                    let stage_name = format!("{:?}", stage).to_lowercase();
                    format!("{}: {}{}ms\\e[0m", stage_name, color, ms)
                })
            })
            .collect();

        format!("\\e[90m[{}ms] {}\\e[0m", total_ms, phase_parts.join(", "))
    }
}

/// 元数据输出Pass
#[derive(Debug)]
pub struct MetadataPass;

impl Default for MetadataPass {
    fn default() -> Self {
        Self::new()
    }
}

impl MetadataPass {
    pub fn new() -> Self {
        Self
    }
}

impl CompilerPass for MetadataPass {
    fn name(&self) -> &'static str {
        "metadata"
    }

    fn run(&mut self, ctx: &mut CompilerContext) -> CEResult<()> {
        ctx.metadata.finalize();
        std::io::stdout().flush().unwrap();
        println!("{}", ctx.metadata.timing_line());
        Ok(())
    }

    fn stage(&self) -> CompilerStage {
        CompilerStage::None
    }

    fn write_output(&self, _ctx: &CompilerContext, _writer: &mut dyn Write) -> CEResult<()> {
        unimplemented!("write output metadata details")
    }
}
