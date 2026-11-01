const { chromium } = require('C:/Users/gyygg/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/playwright');
const { pathToFileURL } = require('url');
const fs = require('fs');
const path = require('path');
const assert = require('assert');

(async () => {
  const root = path.resolve(__dirname, '..');
  const executablePath = ['C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe','C:/Program Files/Google/Chrome/Application/chrome.exe'].find(p => fs.existsSync(p));
  const browser = await chromium.launch({executablePath, headless:true});
  const page = await browser.newPage({viewport:{width:960,height:540}});
  const errors = [];
  page.on('pageerror', e => errors.push(String(e)));
  await page.addInitScript(() => {
    window.testCalls = [];
    window.AndroidClothes = {
      ready() {
        const fields = window.SouthCatalogue.filter(g => g.model === 7).map(g => ({key:g.key,label:g.label,tab:g.tab,min:0,max:g.items.length,val:0}));
        fields.push({key:'skinTone',label:'Tom de pele',tab:'visage',min:1,max:6,val:1});
        fields.push({key:'hairStyle',label:'Cabelo',tab:'cheveux',min:1,max:4,val:1});
        window.testState = {model:163,gender:0,supported:true,canSwitchGender:true,fields};
        window.ClothesApp.onState(window.testState);
      },
      setOption(key,val) {
        window.testCalls.push(['option',key,val]);
        window.testState.fields.find(f => f.key === key).val = val;
        setTimeout(() => window.ClothesApp.onState(JSON.stringify(window.testState)),20);
      },
      camera(key,val) { window.testCalls.push(['camera',key,val]); },
      setTouchBounds(raw) { window.testCalls.push(['bounds',raw]); },
      setGender(val) { window.testCalls.push(['gender',val]); },
      save() { window.testCalls.push(['save']); },
      cancel() { window.testCalls.push(['cancel']); }
    };
  });
  await page.goto(pathToFileURL(path.join(root,'app/src/main/assets/clothes/index.html')).href);
  await page.evaluate(() => document.body.classList.add('preview'));
  assert.equal(await page.locator('.item').count(),140);
  await page.locator('.item').nth(2).click();
  await page.waitForTimeout(60);
  assert.equal(await page.locator('.item.selected').count(),1);
  assert.equal(await page.locator('#piece-number').textContent(),'#003');
  await page.locator('#next').click();
  assert.equal(await page.locator('#piece-number').textContent(),'#004');
  await page.getByRole('button',{name:'Calças',exact:true}).click();
  assert.equal(await page.locator('.item').count(),194);
  await page.locator('#previous').click();
  assert.equal(await page.locator('#piece-number').textContent(),'#194');
  await page.locator('#restore-piece').click();
  assert.equal(await page.locator('#piece-number').textContent(),'ORIGINAL');
  await page.getByRole('button',{name:'Acessórios',exact:true}).click();
  assert.equal(await page.locator('.item').count(),25);
  await page.locator('.item').last().click();
  assert((await page.locator('#piece-count').textContent()).includes('Relógios'));
  await page.locator('#rotation').fill('180'); await page.locator('#rotation').dispatchEvent('input');
  await page.locator('#zoom-in').click();
  await page.waitForTimeout(60);
  assert((await page.evaluate(() => window.testCalls)).some(c => c[0]==='camera' && c[1]==='rotate' && c[2]===180));
  await page.getByRole('button',{name:'Camisas',exact:true}).click();
  fs.mkdirSync(path.join(root,'out/clothes-validation'),{recursive:true});
  for (const [w,h] of [[960,540],[800,360],[1280,720]]) {
    await page.setViewportSize({width:w,height:h});
    await page.waitForTimeout(100);
    const layout = await page.evaluate(() => {
      const rect = id => { const r=document.getElementById(id).getBoundingClientRect(); return {left:r.left,top:r.top,right:r.right,bottom:r.bottom,height:r.height}; };
      return {panel:rect('panel'),items:rect('items'),save:rect('save'),width:innerWidth,height:innerHeight};
    });
    assert(layout.items.height > 30, 'No room for clothing list');
    assert(layout.save.bottom <= h && layout.save.right <= w, 'Save button off screen');
    await page.screenshot({path:path.join(root,`out/clothes-validation/studio-${w}x${h}.png`)});
  }
  await page.locator('#original-mode').click();
  await page.getByRole('button',{name:'Rosto',exact:true}).click();
  assert.equal(await page.locator('.field').count(),1);
  await page.getByRole('button',{name:'Aumentar Tom de pele'}).click();
  assert((await page.locator('output').textContent()).startsWith('2'));
  await page.locator('#save').click();
  assert((await page.evaluate(() => window.testCalls)).some(c => c[0]==='save'));
  assert.deepEqual(errors,[]);
  await browser.close();
  console.log('PASS: catalogue, preview selection, cycling, restore, accessory groups, native bridge, camera, legacy fields, save and 3 landscape layouts.');
})().catch(e => { console.error(e); process.exitCode=1; });
