use clap::Parser as _;
use lycoris::{Settings, args::CompilerArgs, prelude::*};

fn main() {
    // let text = vec![
    //     " 君が持ってきた漫画\n",
    //     "くれた知らない名前のお花\n",
    //     "今日はまだ来ないかな？\n",
    //     "初めての感情知ってしまった\n",
    //     "窓に飾った絵画をなぞってひとりで宇宙を旅して\n",
    //     "それだけでいいはずだったのに\n",
    //     "君の手を握ってしまったら\n",
    //     "孤独を知らないこの街には\n",
    //     "もう二度と帰ってくることはできないのでしょう\n",
    //     "君が手を差し伸べた 光で影が生まれる\n",
    //     "歌って聞かせて この話の続き\n",
    //     "連れて行って見たことない星まで",
    // ];

    // {
    //     let mut writer = BufWriter::new(stdout().lock());
    //     say(&text.join("\n"), 48, &mut writer).unwrap();
    // }

    // ---
    let args = CompilerArgs::parse();
    let settings = Settings::try_from(args).unwrap_or_else(|e| {
        eprintln!("Error initializing settings: {}", e);
        std::process::exit(1)
    });
    let mut pipeline = CompilerPipeline::new(settings);
    // println!("[PIPELINE] {pipeline:#?}");
    match pipeline.run() {
        Ok(ctx) => {
            // 检查IRC失败标志
            if ctx.irc_failed {
                eprintln!("⚠️⚠️⚠️编译终止：IRC图着色失败");
                eprintln!("虚拟汇编已输出到.S文件供调试参考");
                std::process::exit(1);
            }
            // 成功执行
        }
        Err(e) => {
            eprintln!("Compilation error: {}", e);
            std::process::exit(1);
        }
    }
}
