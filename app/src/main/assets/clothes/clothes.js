(function () {
  'use strict';
  var $ = function (id) { return document.getElementById(id); };
  var bridge = window.AndroidClothes, source = window.SouthCatalogue || [];
  var state = {fields:[]}, active = 'haut', focusedKey = '', original = false, zoom = 280, finished = false;
  var genderLock = 0, signature = '', pending = {};
  var categories = [
    ['haut','Hauts','M5 3 2 6l3 4 2-1v12h10V9l2 1 3-4-3-3-4 2h-4Z',73,240],
    ['bas','Bas','M6 3h12l1 18h-6l-1-10-1 10H5Z M6 6h12',39,235],
    ['chaussures','Chaussures','M4 8h6l2 6 8 2v4H3V8Z M3 17h17',8,155],
    ['sacs','Sacs','M8 6V4h8v2 M5 7h14v14H5Z M8 12h8v6H8Z',66,270],
    ['tete','Tête','M3 9c4-3 14-3 18 0l-2 9-7 3-7-3Z M3 10 1 8v7l4 2 M21 10l2-2v7l-4 2',95,140],
    ['bijoux','Bijoux','M9 2h6v5H9Z M9 17h6v5H9Z M7 7h10v10H7Z M12 9v4l2 1',85,175],
    ['cheveux','Cheveux','M4 14V9a8 8 0 0 1 16 0v5 M5 10l5-5 4 4 5 1 M6 12v3a6 6 0 0 0 12 0v-3',97,140],
    ['visage','Visage','M5 4h14v10a7 7 0 0 1-14 0Z M8 10h1 M15 10h1 M10 16h4',97,140],
    ['tatouages','Tatouages','m12 2 3 6 7 1-5 5 1 7-6-3-6 3 1-7-5-5 7-1Z',70,260],
    ['police','Uniforme','M5 3h14v11l-7 8-7-8Z m7 3 2 4 4 1-3 3 1 4-4-2-4 2 1-4-3-3 4-1Z',70,260]
  ];
  var labels = {skinTone:'Teint',face:'Visage',hairStyle:'Coupe',hairColor:'Couleur des cheveux',eyebrowStyle:'Sourcils',eyebrowColor:'Couleur des sourcils',beardStyle:'Barbe',beardColor:'Couleur de la barbe',makeupStyle:'Maquillage',makeupColor:'Teinte du maquillage',topStyle:'Haut',topColor:'Couleur du haut',bottomStyle:'Bas',bottomColor:'Couleur du bas',shoesStyle:'Chaussures',shoesColor:'Couleur des chaussures',hatStyle:'Chapeau',hatColor:'Couleur du chapeau',glasseStyle:'Lunettes',glasseColor:'Couleur des lunettes',maskStyle:'Masque',maskColor:'Couleur du masque',earStyle:'Boucles d\'oreilles',earColor:'Couleur des boucles',clockStyle:'Montre',clockColor:'Couleur de la montre',necklaceStyle:'Collier',necklaceColor:'Couleur du collier',backpackStyle:'Sac à dos',backpackColor:'Couleur du sac',tattooArmLeft:'Tatouage · bras gauche',tattooArmRight:'Tatouage · bras droit',tattooLegLeft:'Tatouage · jambe gauche',tattooLegRight:'Tatouage · jambe droite',tattooBack:'Tatouage · dos',tattooFront:'Tatouage · torse',policeBelt:'Ceinturon',policeVest:'Gilet',policeHat:'Casquette de service'};
  function toast(text) { $('toast').textContent = text; $('toast').style.display = 'block'; clearTimeout(toast.timer); toast.timer = setTimeout(function () { $('toast').style.display = 'none'; }, 2500); }
  function send(method) { if (!bridge || finished) return; try { bridge[method].apply(bridge, Array.prototype.slice.call(arguments,1)); } catch (e) { toast('Impossible d\'envoyer l\'action au jeu.'); } }
  function fields() { return state.fields.filter(function (f) { return f.tab === active; }); }
  function field(key) { return state.fields.find(function (f) { return f.key === key; }); }
  function groups() { var model = state.model === 9 || state.model === 12 ? 9 : 7; return source.filter(function (g) { return g.model === model && fields().some(function (f) { return f.key === g.key; }); }); }
  function currentGroup() { var gs = groups(); return gs.find(function (g) { return g.key === focusedKey; }) || gs[0]; }
  function setOption(key,value) { var f = field(key); if (!f) return; value = Math.max(f.min,Math.min(f.max,value)); f.val = value; pending[key] = value; send('setOption',key,value); if (!bridge) delete pending[key]; render(); }
  function pretty(name) { return name.replace(/_/g,' ').replace(/\b\w/g,function (s) { return s.toUpperCase(); }); }
  function focus() { var c = categories.find(function (c) { return c[0] === active; }); if (!c) return; zoom = c[4]; send('camera','focus',c[3]); send('camera','zoom',zoom); }
  function bounds() { var result = 'viewport=' + innerWidth + ',' + innerHeight; ['panel','categories','camera'].forEach(function (id) { var r = $(id).getBoundingClientRect(); result += ';' + id + '=' + [r.left,r.top,r.right,r.bottom].map(Math.round).join(','); }); send('setTouchBounds',result); }
  function renderNav() { $('categories').replaceChildren(); categories.forEach(function (c) { if (!state.fields.some(function (f) { return f.tab === c[0]; })) return; var b = document.createElement('button'); b.className = 'category' + (active === c[0] ? ' active' : ''); b.innerHTML = '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="' + c[2] + '"></path></svg><span></span>'; b.lastChild.textContent = c[1]; b.setAttribute('aria-pressed',String(active === c[0])); b.onclick = function () { active = c[0]; focusedKey = ''; signature = ''; render(); focus(); }; $('categories').appendChild(b); }); }
  function render() {
    if (!state.fields.some(function (f) { return f.tab === active; }) && state.fields.length) active = state.fields[0].tab;
    renderNav();
    var category = categories.find(function (c) { return c[0] === active; }); $('category-title').textContent = category ? category[1] : 'Ta tenue';
    $('body').value = state.gender || 0; $('body').disabled = !state.canSwitchGender || Date.now() < genderLock;
    $('model-label').textContent = 'PERSONNAGE #' + (state.model || '—');
    var gs = groups(), catalogue = !original && gs.length > 0;
    $('catalog-mode').classList.toggle('active',!original); $('original-mode').classList.toggle('active',original);
    $('selection').hidden = $('items').hidden = $('collection-title').hidden = !catalogue;
    $('fields').style.display = catalogue ? 'none' : 'block';
    $('status').style.display = !state.supported || !state.fields.length ? 'block' : 'none';
    $('status').textContent = !state.fields.length ? 'Chargement de ta garde-robe…' : 'Ce personnage n\'est pas personnalisable. Choisis un autre corps.';
    $('save').disabled = !state.supported || !state.fields.length;
    var nextSignature = active + ':' + focusedKey + ':' + catalogue + ':' + state.model + ':' + JSON.stringify(fields());
    if (nextSignature === signature) return; signature = nextSignature;
    if (catalogue) {
      var chosen = currentGroup(), selected = field(chosen.key).val, piece = chosen.items[selected-1];
      $('piece-name').textContent = piece ? pretty(piece.name) : 'Tenue actuelle';
      $('piece-number').textContent = piece ? '#' + String(selected).padStart(3,'0') : 'ORIGINAL';
      $('piece-count').textContent = chosen.items.length + ' pièces · ' + chosen.label;
      var scroll = $('items').scrollTop; $('items').replaceChildren();
      gs.forEach(function (g) { var f = field(g.key); g.items.forEach(function (item,index) {
        var b = document.createElement('button'); b.className = 'item' + (f.val === index+1 ? ' selected' : ''); b.setAttribute('aria-pressed',String(f.val === index+1));
        var img = document.createElement('img'); img.src = 'south/' + item.texture; img.loading = 'lazy'; img.alt = ''; b.appendChild(img);
        var number = document.createElement('small'); number.textContent = '#' + String(index+1).padStart(3,'0'); b.appendChild(number);
        var label = document.createElement('span'); label.textContent = pretty(item.name); b.appendChild(label); b.title = g.label + ' · ' + label.textContent;
        b.onclick = function () { focusedKey = g.key; setOption(g.key,index+1); }; $('items').appendChild(b);
      }); }); $('items').scrollTop = scroll;
    } else {
      $('fields').replaceChildren(); fields().filter(function (f) { return f.key.indexOf('south') !== 0; }).forEach(function (f) {
        var div = document.createElement('div'); div.className = 'field'; var label = document.createElement('label'); label.textContent = labels[f.key] || f.label; div.appendChild(label);
        var controls = document.createElement('div'); controls.className = 'field-controls'; var prev = document.createElement('button'), next = document.createElement('button'), value = document.createElement('output');
        prev.textContent = '−'; next.textContent = '+'; value.textContent = f.val + ' / ' + f.max;
        prev.setAttribute('aria-label','Diminuer ' + label.textContent); next.setAttribute('aria-label','Augmenter ' + label.textContent);
        prev.disabled = f.val <= f.min; next.disabled = f.val >= f.max;
        prev.onclick = function () { setOption(f.key,f.val-1); }; next.onclick = function () { setOption(f.key,f.val+1); };
        controls.append(prev,value,next); div.appendChild(controls); $('fields').appendChild(div);
      });
    }
  }
  window.ClothesApp = {onState:function (json) {
    var next; try { next = typeof json === 'string' ? JSON.parse(json) : json; } catch (e) { toast('Données du personnage illisibles.'); return; }
    if (!next || !Array.isArray(next.fields)) return;
    if (state.model !== next.model) pending = {};
    Object.keys(pending).forEach(function (key) { var f = next.fields.find(function (f) { return f.key === key; }); if (!f || f.val === pending[key]) delete pending[key]; else f.val = pending[key]; });
    state = next; render(); bounds();
  }};
  $('catalog-mode').onclick = function () { original = false; signature = ''; render(); };
  $('original-mode').onclick = function () { original = true; signature = ''; render(); };
  $('previous').onclick = function () { var g = currentGroup(); if (!g) return; var f = field(g.key); setOption(g.key,f.val <= 1 ? g.items.length : f.val-1); };
  $('next').onclick = function () { var g = currentGroup(); if (!g) return; var f = field(g.key); setOption(g.key,f.val >= g.items.length ? 1 : f.val+1); };
  $('restore-piece').onclick = function () { var g = currentGroup(); if (g) setOption(g.key,0); };
  $('body').onchange = function () { if (Date.now() < genderLock) return; genderLock = Date.now()+1500; pending = {}; send('setGender',Number(this.value)); this.disabled = true; setTimeout(function () { $('body').disabled = !state.canSwitchGender; },1550); };
  var frame = 0; $('rotation').oninput = function () { if (frame) return; frame = requestAnimationFrame(function () { frame = 0; send('camera','rotate',Number($('rotation').value)); }); };
  $('zoom-in').onclick = function () { zoom = Math.max(100,zoom-20); send('camera','zoom',zoom); };
  $('zoom-out').onclick = function () { zoom = Math.min(450,zoom+20); send('camera','zoom',zoom); };
  $('reset-camera').onclick = function () { $('rotation').value = 0; send('camera','rotate',0); focus(); };
  function cancel() { if (finished) return; send('cancel'); finished = true; }
  $('close').onclick = $('cancel').onclick = cancel;
  $('save').onclick = function () { if (finished || !state.supported) return; send('save'); finished = true; };
  document.addEventListener('keydown',function (e) { if (e.key === 'Escape') cancel(); }); window.addEventListener('resize',bounds);
  if (!bridge) {
    document.body.classList.add('preview');
    var mock = source.filter(function (g) { return g.model === 7; }).map(function (g) { return {key:g.key,label:g.label,tab:g.tab,min:0,max:g.items.length,val:0}; });
    mock.push({key:'skinTone',label:'Teint',tab:'visage',min:1,max:6,val:1});
    window.ClothesApp.onState({model:163,gender:0,canSwitchGender:true,supported:true,fields:mock});
  }
  render(); bounds(); send('ready');
})();
