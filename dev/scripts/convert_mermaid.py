#!/usr/bin/env python3
import requests
import base64
import zlib
import re
import glob
from pathlib import Path

def extract_mermaid_blocks(md_content):
    """Extract all Mermaid code blocks from markdown content."""
    pattern = r'```\s*mermaid\s*\n(.*?)```'
    matches = re.findall(pattern, md_content, re.DOTALL)
    return [match.strip() for match in matches]

def encode_mermaid(mermaid_code):
    """Encode Mermaid code for Kroki API."""
    compressed = zlib.compress(mermaid_code.encode('utf-8'), 9)
    encoded = base64.urlsafe_b64encode(compressed).decode('utf-8')
    return encoded

def test_simple_diagram():
    """Test with a simple diagram first"""
    simple = """
graph TD
    A[Start] --> B[End]
"""
    encoded = encode_mermaid(simple)
    url = f"http://localhost:8000/mermaid/svg/{encoded}"
    
    try:
        response = requests.get(url, timeout=10)
        response.raise_for_status()
        print("✓ Simple diagram works!")
        return True
    except Exception as e:
        print(f"✗ Even simple diagram fails: {e}")
        return False

def convert_mermaid_via_kroki(mermaid_code, output_file, format='svg'):
    """Convert Mermaid to image using Kroki API."""
    try:
        encoded = encode_mermaid(mermaid_code)
        url = f"http://localhost:8000/mermaid/{format}/{encoded}"
        
        response = requests.get(url, timeout=60)  # Increased timeout
        
        if response.status_code == 503:
            print(f"✗ Kroki returned 503 - diagram may have syntax errors")
            # Show first 100 chars of diagram for debugging
            print(f"  Diagram preview: {mermaid_code[:100]}...")
            return False
            
        response.raise_for_status()
        
        with open(output_file, 'wb') as f:
            f.write(response.content)
        
        print(f"✓ Created {output_file}")
        return True
    except requests.exceptions.HTTPError as e:
        print(f"✗ HTTP Error {e.response.status_code}: {e}")
        print(f"  Diagram preview: {mermaid_code[:100]}...")
        return False
    except Exception as e:
        print(f"✗ Error: {e}")
        return False

def convert_md_file(input_file, output_format='svg'):
    """Extract and convert Mermaid diagrams from .md file."""
    input_path = Path(input_file)
    
    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            md_content = f.read()
        
        mermaid_blocks = extract_mermaid_blocks(md_content)
        
        if not mermaid_blocks:
            print(f"⊘ No Mermaid diagrams in {input_file}")
            return
        
        print(f"\nProcessing {input_file} ({len(mermaid_blocks)} diagram(s))")
        
        for idx, mermaid_code in enumerate(mermaid_blocks, start=1):
            if len(mermaid_blocks) == 1:
                output_file = input_path.with_suffix(f'.md.{output_format}')
            else:
                output_file = input_path.parent / f"{input_path.stem}_{idx}.md.{output_format}"
            
            print(f"  Diagram {idx}/{len(mermaid_blocks)}...", end=" ")
            convert_mermaid_via_kroki(mermaid_code, output_file, output_format)
    
    except Exception as e:
        print(f"✗ Error processing {input_file}: {e}")

def convert_all_md_files(root_dir='.', output_format='svg'):
    """Recursively convert all .md files with Mermaid diagrams."""
    
    # First test with simple diagram
    print("Testing Kroki connection...")
    if not test_simple_diagram():
        print("Cannot proceed - Kroki not responding correctly")
        return
    
    md_files = glob.glob(f"{root_dir}/**/*.md", recursive=True)
    
    print(f"\nFound {len(md_files)} markdown file(s)")
    print("="*50)
    
    success = 0
    failed = 0
    
    for md_file in md_files:
        # Count successes and failures
        with open(md_file, 'r') as f:
            blocks = len(extract_mermaid_blocks(f.read()))
        
        convert_md_file(md_file, output_format)
    
    print("="*50)

if __name__ == '__main__':
    convert_all_md_files('/Users/clemens/Documents/code/SPI_FullDuplex_ComDMA/doc/images', output_format='svg')