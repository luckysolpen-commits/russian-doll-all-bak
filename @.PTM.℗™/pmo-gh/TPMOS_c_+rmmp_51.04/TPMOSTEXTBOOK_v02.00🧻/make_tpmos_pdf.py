import os
import re
from fpdf import FPDF
from fpdf.enums import XPos, YPos

# CONFIGURATION
BASE_DIR = "#.docs/TPMOSTEXTBOOK_v01.01" # Using v01.01 as fallback if v02.00 doesn't exist, but prompt says v02.00
if not os.path.exists(BASE_DIR):
    BASE_DIR = "#.docs/TPMOSTEXTBOOK_v02.00🧻"

OUTPUT_PDF = "TPMOS_TEXTBOOK_v02.00.pdf"
FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
FONT_BOLD_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"

class TPMOS_PDF(FPDF):
    def header(self):
        if self.page_no() > 1:
            self.set_font("DejaVu", "B", 10)
            self.set_text_color(100, 100, 100)
            self.set_x(0)
            self.cell(210, 10, "📚 TPMOS TEXTBOOK v02.00 🎓  ", align="R", new_x=XPos.LMARGIN, new_y=YPos.NEXT)
            self.ln(5)

    def footer(self):
        self.set_y(-15)
        self.set_font("DejaVu", "", 8)
        self.set_text_color(150, 150, 150)
        self.cell(0, 10, f"Page {self.page_no()}", align="C")

def create_pdf():
    pdf = TPMOS_PDF()
    
    # Add Unicode font support
    pdf.add_font("DejaVu", "", FONT_PATH)
    pdf.add_font("DejaVu", "B", FONT_BOLD_PATH)
    pdf.set_font("DejaVu", size=12)
    pdf.set_auto_page_break(auto=True, margin=15)

    # 1. COVER PAGE
    pdf.add_page()
    pdf.set_font("DejaVu", "B", 24)
    pdf.ln(60)
    pdf.cell(0, 20, "📚 TPMOS_TEXTBOOK 🎓", align="C", new_x=XPos.LMARGIN, new_y=YPos.NEXT)
    pdf.set_font("DejaVu", "B", 16)
    pdf.cell(0, 10, "The Definitive Guide to the Mono-OS", align="C", new_x=XPos.LMARGIN, new_y=YPos.NEXT)
    pdf.ln(10)
    pdf.set_font("DejaVu", "", 12)
    pdf.cell(0, 10, "Version 02.00 (Standardized Ops Edition)", align="C", new_x=XPos.LMARGIN, new_y=YPos.NEXT)
    pdf.ln(80)
    pdf.set_font("DejaVu", "", 10)
    pdf.cell(0, 10, "Softness wins. The empty center of the flexbox holds ten thousand things. 🧘‍♂️", align="C", new_x=XPos.LMARGIN, new_y=YPos.NEXT)

    # 2. READ INDEX
    index_path = os.path.join(BASE_DIR, "INDEX.md")
    with open(index_path, "r", encoding="utf-8") as f:
        index_content = f.read()

    # Extract chapter links in order
    chapters = re.findall(r"\[(.*?)\]\((.*?\.md)\)", index_content)
    
    # 3. ADD CHAPTERS
    for title, filename in chapters:
        if filename == "INDEX.md": continue
        
        file_path = os.path.join(BASE_DIR, filename)
        if not os.path.exists(file_path):
            print(f"Warning: {filename} not found.")
            continue

        pdf.add_page()
        with open(file_path, "r", encoding="utf-8") as f:
            content = f.read()

        lines = content.split("\n")
        for line in lines:
            line = line.strip()
            pdf.set_x(pdf.l_margin) # Reset X to left margin for every line
            
            if not line:
                pdf.ln(5)
                continue
            
            if line.startswith("---"):
                pdf.ln(2)
                pdf.set_draw_color(200, 200, 200)
                pdf.line(pdf.l_margin, pdf.get_y(), 210 - pdf.r_margin, pdf.get_y())
                pdf.ln(2)
                continue

            if line.startswith("# "):
                pdf.set_font("DejaVu", "B", 18)
                pdf.multi_cell(0, 10, line[2:], new_x=XPos.LMARGIN, new_y=YPos.NEXT)
                pdf.ln(5)
            elif line.startswith("## "):
                pdf.set_font("DejaVu", "B", 14)
                pdf.multi_cell(0, 10, line[3:], new_x=XPos.LMARGIN, new_y=YPos.NEXT)
                pdf.ln(3)
            elif line.startswith("### "):
                pdf.set_font("DejaVu", "B", 12)
                pdf.multi_cell(0, 10, line[4:], new_x=XPos.LMARGIN, new_y=YPos.NEXT)
                pdf.ln(2)
            elif line.startswith("* "):
                pdf.set_font("DejaVu", "", 12)
                pdf.multi_cell(0, 8, f"  • {line[2:]}", new_x=XPos.LMARGIN, new_y=YPos.NEXT)
            elif line.startswith("> "):
                pdf.set_font("DejaVu", "B", 11) # Use Bold instead of Italic
                pdf.set_text_color(80, 80, 80)
                pdf.multi_cell(0, 8, line[2:], new_x=XPos.LMARGIN, new_y=YPos.NEXT)
                pdf.set_text_color(0, 0, 0)
            else:
                pdf.set_font("DejaVu", "", 12)
                clean_line = line.replace("**", "").replace("*", "")
                pdf.multi_cell(0, 8, clean_line, new_x=XPos.LMARGIN, new_y=YPos.NEXT)

    # 4. SAVE PDF
    print(f"Generating {OUTPUT_PDF}...")
    pdf.output(OUTPUT_PDF)
    print("Done! ✅")

if __name__ == "__main__":
    create_pdf()
