import os
import re
from fpdf import FPDF
from fpdf.enums import XPos, YPos

# CONFIGURATION
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BASE_DIR = SCRIPT_DIR
OUTPUT_PDF = os.path.join(SCRIPT_DIR, "TPMOS_TEXTBOOK_v02.03.pdf")

# PATH TO CHINESE-SUPPORTING FONT (Found via fc-list)
# Using UMing as it is a common TrueType collection often accessible directly
FONT_PATH = "/usr/share/fonts/truetype/arphic/uming.ttc" 

# EMOJI SUBSTITUTION MAPPING (With Chinese Characters for learning)
EMOJI_MAP = {
    "📚": "[B 书]", "📖": "[B 阅]", "🎓": "[GRAD 学]", "🚀": "[LAUNCH 发]", "🌌": "[COSMOS 宇]", 
    "🌀": "[REC 归]", "🏛": "[BLD 馆]", "🏗": "[BLD 建]", "🤖": "[BOT 机]", "🧬": "[DNA 命]", 
    "🔬": "[SCI 究]", "🧪": "[LAB 验]", "🌍": "[EARTH 地]", "🌎": "[EARTH 球]", "🛡": "[SEC 安]", 
    "⛓": "[NET 链]", "🖥": "[CPU 算]", "🔋": "[PWR 电]", "🧠": "[MIND 脑]", "🎭": "[VIEW 戏]", 
    "💰": "[$ 钱]", "💎": "[VAL 宝]", "⚖": "[LAW 法]", "💼": "[BIZ 业]", "📈": "[MKT↑ 升]", 
    "📉": "[MKT↓ 降]", "📣": "[PR 宣]", "🤝": "[NET 协]", "🪄": "[PEN 笔]", "💓": "[PULSE 心]", 
    "🛠": "[TOOL 工]", "⚙": "[TOOL 齿]", "🎨": "[GL 艺]", "📝": "[DOC 记]", "📋": "[DOC 单]", 
    "📺": "[TERM 视]", "📟": "[TERM 显]", "🗺": "[MAP 图]", "🚪": "[EXIT 门]", "⚔": "[WAR 战]", 
    "🧘‍♂️": "[ZEN 禅]", "🌟": "[* 星]", "✨": "[* 闪]", "🕯": "[IMM 烛]", "🍼": "[KISS 奶]", 
    "⚡": "[FAST 快]", "🌓": "[MODE 模式]", "🏃": "[SPRINT 跑]", "👣": "[PATH 迹]", "🏭": "[APP 厂]", 
    "🐾": "[PET 爪]", "👤": "[USER 人]", "💨": "[FAST 速]", "🌉": "[BRIDGE 桥]", "👻": "[GHOST 鬼]", 
    "🏘": "[COM 村]", "🌑": "[LUNA 阴]", "🌕": "[LUNA 阳]", "🛰": "[EXP 卫]", "📐": "[GEOM 角]", 
    "🏁": "[GO 终]", "🎰": "[LUCK 赌]", "🐠": "[SEA 鱼]", "✅": "[OK 是]", "❌": "[FAIL 错]", 
    "💡": "[TIPS 明]", "⚠": "[WARN 警]", "♾": "[INF 无]", "🧹": "[CLEAN 洁]", "💤": "[IDLE 眠]", 
    "😴": "[IDLE 睡]", "🚫": "[NO 禁]", "🧨": "[FIRE 炮]", "🏢": "[CORP 司]", "⛩": "[GATE 门]",
    "🖇": "[LINK 连]", "🔗": "[LINK 接]", "👁": "[EYE 眼]", "✎": "[PEN 笔]", "⚐": "[GO 旗]",
    "◑": "[MODE 态]", "◎": "[REC 圆]", "⌂": "[BLD 宅]", "☤": "[DNA 医]", "⚗": "[SCI 壶]",
    "⊕": "[EARTH 环]", "⛨": "[SEC 盾]", "⌨": "[CPU 键]", "☍": "[IDLE 连]", "⎚": "[TERM 屏]",
    "⌗": "[MAP 井]", "☯": "[ZEN 道]", "∞": "[INF 穷]", "☩": "[CLEAN 十]", "⦸": "[NO 止]",
    "⎗": "[OUT 出]", "⊿": "[GEOM 形]", "⚄": "[LUCK 点]", "🐟": "[SEA 鱼]", "🪲": "[BUG 虫]",
    "☑": "[OK 对]", "☒": "[FAIL 误]", "📢": "[PR 喇]", "🕮": "[B 书]", "🪞": "[MIRROR 镜]",
    "🧱": "[PIECE 砖]", "♂": "", "♀": "", "‍": "", "️": ""
}

def clean_emojis(text):
    for emoji, sub in EMOJI_MAP.items():
        text = text.replace(emoji, sub)
    # Filter to only allow standard ASCII + CJK range (Simplified/Traditional)
    # CJK range is roughly \u4e00-\u9fff
    return text

class TPMOS_PDF(FPDF):
    def header(self):
        if self.page_no() > 1:
            self.set_font("Hanzi", "B", 10)
            self.set_text_color(100, 100, 100)
            self.set_x(0)
            header_text = clean_emojis("📚 TPMOS TEXTBOOK v02.03 🎓  ")
            self.cell(210, 10, header_text, align="R", new_x=XPos.LMARGIN, new_y=YPos.NEXT)
            self.ln(5)

    def footer(self):
        self.set_y(-15)
        self.set_font("Hanzi", "", 8)
        self.set_text_color(150, 150, 150)
        self.cell(0, 10, f"Page {self.page_no()}", align="C")

def create_pdf():
    pdf = TPMOS_PDF()
    
    # Use UMing for both English and Chinese
    pdf.add_font("Hanzi", "", FONT_PATH)
    pdf.add_font("Hanzi", "B", FONT_PATH) # UMing often doesn't have a separate bold, so we reuse
    pdf.set_font("Hanzi", size=12)
    pdf.set_auto_page_break(auto=True, margin=15)

    # 1. COVER PAGE
    pdf.add_page()
    pdf.set_font("Hanzi", "B", 24)
    pdf.ln(60)
    pdf.cell(0, 20, clean_emojis("📚 TPMOS_TEXTBOOK 🎓"), align="C", new_x=XPos.LMARGIN, new_y=YPos.NEXT)
    pdf.set_font("Hanzi", "B", 16)
    pdf.cell(0, 10, clean_emojis("The Definitive Guide to the Mono-OS (基于件的系统指南)"), align="C", new_x=XPos.LMARGIN, new_y=YPos.NEXT)
    pdf.ln(10)
    pdf.set_font("Hanzi", "", 12)
    pdf.cell(0, 10, "Version 02.02 (Standardized Ops Edition)", align="C", new_x=XPos.LMARGIN, new_y=YPos.NEXT)
    pdf.ln(80)
    pdf.set_font("Hanzi", "", 10)
    pdf.cell(0, 10, clean_emojis("Softness wins. The empty center of the flexbox holds ten thousand things. 🧘‍♂️"), align="C", new_x=XPos.LMARGIN, new_y=YPos.NEXT)

    # 2. READ INDEX & ADD TOC PAGE
    index_path = os.path.join(BASE_DIR, "INDEX.md")
    with open(index_path, "r", encoding="utf-8") as f:
        index_content = f.read()

    pdf.add_page()
    pdf.set_font("Hanzi", "B", 18)
    pdf.cell(0, 10, clean_emojis("📋 Table of Contents (目录)"), align="L", new_x=XPos.LMARGIN, new_y=YPos.NEXT)
    pdf.ln(5)
    pdf.set_font("Hanzi", "", 12)
    
    index_lines = index_content.split("\n")
    for line in index_lines:
        line = line.strip()
        if not line or line.startswith("# ") or "Dependency Graph" in line or "🕸️" in line or "```" in line or "graph TD" in line or "subgraph" in line or "style" in line:
            continue
        
        match = re.search(r"\[(.*?)\]", line)
        if match:
            title = clean_emojis(match.group(1))
            pdf.set_x(pdf.l_margin + 5)
            pdf.multi_cell(0, 8, f"• {title}", new_x=XPos.LMARGIN, new_y=YPos.NEXT)
        elif line.startswith("### "):
            pdf.ln(3)
            pdf.set_font("Hanzi", "B", 14)
            pdf.cell(0, 10, clean_emojis(line[4:]), new_x=XPos.LMARGIN, new_y=YPos.NEXT)
            pdf.set_font("Hanzi", "", 12)

    chapters = re.findall(r"\[(.*?)\]\((.*?\.md)\)", index_content)
    
    # 3. ADD CHAPTERS
    for title, filename in chapters:
        if filename == "INDEX.md": continue
        
        file_path = os.path.join(BASE_DIR, filename)
        if not os.path.exists(file_path):
            continue

        pdf.add_page()
        with open(file_path, "r", encoding="utf-8") as f:
            content = f.read()

        lines = content.split("\n")
        for line in lines:
            line = line.strip()
            pdf.set_x(pdf.l_margin)
            
            if not line:
                pdf.ln(5)
                continue
            
            if line.startswith("---"):
                pdf.ln(2)
                pdf.set_draw_color(200, 200, 200)
                pdf.line(pdf.l_margin, pdf.get_y(), 210 - pdf.r_margin, pdf.get_y())
                pdf.ln(2)
                continue

            clean_line = clean_emojis(line)

            if line.startswith("# "):
                pdf.set_font("Hanzi", "B", 18)
                pdf.multi_cell(0, 10, clean_line[2:], new_x=XPos.LMARGIN, new_y=YPos.NEXT)
                pdf.ln(5)
            elif line.startswith("## "):
                pdf.set_font("Hanzi", "B", 14)
                pdf.multi_cell(0, 10, clean_line[3:], new_x=XPos.LMARGIN, new_y=YPos.NEXT)
                pdf.ln(3)
            elif line.startswith("### "):
                pdf.set_font("Hanzi", "B", 12)
                pdf.multi_cell(0, 10, clean_line[4:], new_x=XPos.LMARGIN, new_y=YPos.NEXT)
                pdf.ln(2)
            elif line.startswith("* "):
                pdf.set_font("Hanzi", "", 12)
                pdf.multi_cell(0, 8, f"  • {clean_line[2:]}", new_x=XPos.LMARGIN, new_y=YPos.NEXT)
            elif line.startswith("> "):
                pdf.set_font("Hanzi", "B", 11)
                pdf.set_text_color(80, 80, 80)
                pdf.multi_cell(0, 8, clean_line[2:], new_x=XPos.LMARGIN, new_y=YPos.NEXT)
                pdf.set_text_color(0, 0, 0)
            else:
                pdf.set_font("Hanzi", "", 12)
                clean_line_final = clean_line.replace("**", "").replace("*", "")
                pdf.multi_cell(0, 8, clean_line_final, new_x=XPos.LMARGIN, new_y=YPos.NEXT)

    # 4. SAVE PDF
    print(f"Generating {OUTPUT_PDF}...")
    pdf.output(OUTPUT_PDF)
    print("Done! ✅")

if __name__ == "__main__":
    create_pdf()
