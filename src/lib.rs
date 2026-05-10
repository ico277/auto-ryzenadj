use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Debug)]
pub struct DaemonConfigDaemon {
    pub loop_delay_ms: Option<u64>,
    #[serde(rename = "default")]
    pub default_profile: Option<String>,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct DaemonConfigProfile {
    pub name: String,
    pub executable: String,
    pub arguments: Vec<String>,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct DaemonConfig {
    pub daemon: DaemonConfigDaemon,
    pub profiles: Vec<DaemonConfigProfile>
}