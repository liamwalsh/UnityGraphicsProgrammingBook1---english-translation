const puppeteer = require('puppeteer');
const fs = require('fs');
const path = require('path');

const pdfDir = '/Users/liamwalsh/Projects/Personal/UnityGraphicsProgramming_WithClaude/PDFs';

async function generatePDF(htmlFile, pdfFile) {
    const browser = await puppeteer.launch({ headless: 'new' });
    const page = await browser.newPage();

    const htmlPath = path.join(pdfDir, htmlFile);
    const pdfPath = path.join(pdfDir, pdfFile);

    await page.goto(`file://${htmlPath}`, { waitUntil: 'networkidle0' });

    await page.pdf({
        path: pdfPath,
        format: 'A4',
        margin: { top: '20mm', bottom: '20mm', left: '15mm', right: '15mm' },
        printBackground: true,
        displayHeaderFooter: true,
        headerTemplate: '<div></div>',
        footerTemplate: '<div style="font-size: 10px; text-align: center; width: 100%;"><span class="pageNumber"></span> / <span class="totalPages"></span></div>'
    });

    await browser.close();
    console.log(`Created: ${pdfFile}`);
}

async function main() {
    const volumes = [
        ['Volume1_UnityGraphicsProgramming.html', 'Volume1_UnityGraphicsProgramming.pdf'],
        ['Volume2_UnityGraphicsProgramming.html', 'Volume2_UnityGraphicsProgramming.pdf'],
        ['Volume3_UnityGraphicsProgramming.html', 'Volume3_UnityGraphicsProgramming.pdf'],
        ['Volume4_UnityGraphicsProgramming.html', 'Volume4_UnityGraphicsProgramming.pdf']
    ];

    for (const [html, pdf] of volumes) {
        await generatePDF(html, pdf);
    }

    console.log('All PDFs generated!');
}

main().catch(console.error);
