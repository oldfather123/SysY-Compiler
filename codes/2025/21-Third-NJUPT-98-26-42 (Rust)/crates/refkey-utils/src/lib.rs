use proc_macro::TokenStream;
use quote::quote;
use syn::{parse_macro_input, DeriveInput};

/// 为 slotmap Key 类型自动实现 Display trait 和工具方法
#[proc_macro_derive(RefUtils)]
pub fn derive_ref_utils(input: TokenStream) -> TokenStream {
    let input = parse_macro_input!(input as DeriveInput);
    let name = &input.ident;

    // 提取类型名称作为前缀（去掉 Ref 后缀）
    // let type_name = name.to_string();
    // let prefix = if type_name.ends_with("Ref") {
    //     &type_name[..type_name.len() - 3]
    // } else {
    //     &type_name
    // };

    let expanded = quote! {
        impl std::fmt::Display for #name {
            fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
                let (idx, version) = self.index_ver();
                // write!(f, "{}({}:{})", #prefix, idx, version)
                if version == 1{
                    write!(f, "{}", idx)
                }else{
                    write!(f, "{}:{}", idx, version)
                }
            }
        }

        impl #name {
            /// 获取 RefKey 序号
            pub fn index(&self) -> u32 {
                self.data().as_ffi() as u32
            }

            /// 获取 RefKey 版本
            pub fn version(&self) -> u32 {
                ((self.data().as_ffi() >> 32) | 1) as u32
            }

            /// 获取 RefKey 元组
            pub fn index_ver(&self) -> (u32, u32) {
                let value = self.data().as_ffi();
                let idx = value & 0xffff_ffff;
                let version = (value >> 32) | 1; // Ensure version is odd.
                (idx as u32, version as u32)
            }

            pub fn as_ffi(&self) -> u64 {
                self.data().as_ffi()
            }

            pub fn from_ffi(value: u64) -> Self {
                // 使用 slotmap 的 from_ffi 方法
                use slotmap::Key;
                Self::from(slotmap::KeyData::from_ffi(value))
            }
        }
    };

    TokenStream::from(expanded)
}
