use std::fs;

use ryzenprofiles::DaemonConfig;

fn main() {
    let config: DaemonConfig = toml::from_str(&fs::read_to_string("./config.toml").unwrap()).unwrap();

    println!("{:#?}", config);
}
