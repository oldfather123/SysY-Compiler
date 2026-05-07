#[derive(Debug)]
pub struct ATNDeserializationOptions {
    _read_only: bool,
    verify_atn: bool,
    _generate_rule_bypass_transitions: bool,
}

impl ATNDeserializationOptions {
    pub fn is_verify(&self) -> bool {
        self.verify_atn
    }
}

impl Default for ATNDeserializationOptions {
    fn default() -> Self {
        ATNDeserializationOptions {
            _read_only: true,
            verify_atn: true,
            _generate_rule_bypass_transitions: false,
        }
    }
}
