import pdfplumber
import sys

sys.stdout.reconfigure(encoding='utf-8')

pdf = pdfplumber.open(r'D:\Desktop\Processor design studio\processor-design-studio-spec.pdf')
for i, page in enumerate(pdf.pages):
    print(f'--- Page {i+1} ---')
    text = page.extract_text()
    if text:
        print(text)
    print()
pdf.close()
