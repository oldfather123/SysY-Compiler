use std::fs;

const TEST_DIR: &str = "../../test/";
const FUNCTIONAL: &str = "functional/";
const HOMEMADE: &str = "homemade/";

pub struct TestFileIter {
    files: Vec<String>,
    index: usize,
}

impl TestFileIter {
    pub fn functional() -> Self {
        Self::new(TEST_DIR.to_owned() + FUNCTIONAL)
    }
    pub fn homemade() -> Self {
        Self::new(TEST_DIR.to_owned() + HOMEMADE)
    }

    pub fn new(dir: String) -> Self {
        let mut files = vec![];
        if let Ok(entries) = fs::read_dir(dir) {
            for entry in entries {
                if let Ok(entry) = entry {
                    let path = entry.path().to_str().unwrap().to_owned();
                    if path.ends_with(".sy") {
                        files.push(path);
                    }
                }
            }
        }
        Self { files, index: 0 }
    }
}

impl Iterator for TestFileIter {
    type Item = String;
    fn next(&mut self) -> Option<Self::Item> {
        if self.index < self.files.len() {
            let path = self.files[self.index].clone();
            self.index += 1;
            Some(path)
        } else {
            None
        }
    }
}

#[test]
fn main() {
    for path in TestFileIter::homemade() {
        println!("{}", path);
    }
}
