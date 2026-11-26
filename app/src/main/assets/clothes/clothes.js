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
  /* [v46] vrais vetements : silhouette (t-shirt, sweat, jean, basket...) remplie avec le motif */
  var SH = {
    tee: ['M31 16 L43 11 C46 16 54 16 57 11 L69 16 L89 33 L79 45 L70 38 L70 91 L30 91 L30 38 L21 45 L11 33 Z', 'M43 11 C46 19 54 19 57 11 M30 38 L30 42 M70 38 L70 42'],
    long: ['M31 16 L43 11 C46 16 54 16 57 11 L69 16 L81 27 L93 76 L83 79 L72 42 L71 91 L29 91 L28 42 L17 79 L7 76 L19 27 Z', 'M43 11 C46 19 54 19 57 11 M8.5 71 L18.5 74 M91.5 71 L81.5 74'],
    sweat: ['M31 16 L43 11 C46 16 54 16 57 11 L69 16 L81 27 L93 76 L83 79 L72 42 L71 91 L29 91 L28 42 L17 79 L7 76 L19 27 Z', 'M43 11 C46 19 54 19 57 11 M29 85 L71 85 M8.5 71 L18.5 74 M91.5 71 L81.5 74 M42 46 L58 46 L60 62 L40 62 Z'],
    tank: ['M35 9 L41 9 C43 24 57 24 59 9 L65 9 C65 22 70 30 73 34 L73 91 L27 91 L27 34 C30 30 35 22 35 9 Z', 'M41 9 C43 26 57 26 59 9'],
    crop: ['M31 22 L43 17 C46 22 54 22 57 17 L69 22 L87 37 L78 47 L70 41 L70 72 L30 72 L30 41 L22 47 L13 37 Z', 'M43 17 C46 25 54 25 57 17 M30 67 L70 67'],
    bra: ['M16 40 C20 28 34 26 41 33 L50 42 L59 33 C66 26 80 28 84 40 L86 56 C72 60 60 58 50 51 C40 58 28 60 14 56 Z', 'M30 30 L35 10 M70 30 L65 10'],
    pants: ['M28 8 L72 8 L76 93 L56 93 L50 38 L44 93 L24 93 Z', 'M28 15 L72 15 M50 15 L50 30 M33 15 L33 24'],
    shorts: ['M26 18 L74 18 L80 68 L56 72 L50 42 L44 72 L20 68 Z', 'M26 25 L74 25 M50 25 L50 36'],
    brief: ['M20 30 L80 30 L76 46 C66 52 60 60 57 70 L43 70 C40 60 34 52 24 46 Z', 'M20 36 L80 36'],
    sneaker: ['M8 64 C9 50 16 44 26 42 L38 38 C44 46 52 48 60 48 L74 52 C86 55 92 60 92 68 L92 75 L8 75 Z', 'M8 68 L92 68 M40 41 L45 50 M47 43 L51 51 M54 45 L57 51'],
    boot: ['M22 16 L48 16 L50 52 L76 58 C88 61 92 66 92 72 L92 79 L20 79 Z', 'M20 72 L92 72 M24 24 L46 24 M30 30 L44 30 M30 38 L44 38'],
    slide: ['M8 71 C8 63 20 59 50 59 C80 59 92 63 92 71 C92 77 80 79 50 79 C20 79 8 77 8 71 Z M24 61 C28 42 72 42 76 61 Z', 'M24 61 C28 42 72 42 76 61'],
    backpack: ['M28 22 C28 12 72 12 72 22 L76 88 C76 92 72 94 68 94 L32 94 C28 94 24 92 24 88 Z', 'M40 15 C40 4 60 4 60 15 M32 58 L68 58 L66 84 L34 84 Z M32 66 L68 66'],
    shoulder: ['M18 34 L82 34 L86 82 L14 82 Z', 'M24 34 C24 6 76 6 76 34 M18 48 L82 48'],
    travel: ['M8 42 C8 34 92 34 92 42 L92 80 C92 87 8 87 8 80 Z', 'M34 36 C34 18 66 18 66 36 M8 53 L92 53 M50 53 L50 60'],
    mask: ['M12 38 C34 28 66 28 88 38 L84 62 C72 80 28 80 16 62 Z', 'M12 38 L4 30 M88 38 L96 30 M20 48 L80 48 M22 58 L78 58'],
    ear: ['M14 56 a15 15 0 1 0 30 0 a15 15 0 1 0 -30 0 Z M20 56 a9 9 0 1 0 18 0 a9 9 0 1 0 -18 0 Z M56 56 a15 15 0 1 0 30 0 a15 15 0 1 0 -30 0 Z M62 56 a9 9 0 1 0 18 0 a9 9 0 1 0 -18 0 Z', 'M29 30 L29 41 M71 30 L71 41'],
    watch: ['M38 3 L62 3 L62 97 L38 97 Z M50 25 a25 25 0 1 0 0.01 0 Z', 'M50 30 a20 20 0 1 0 0.01 0 M50 40 L50 50 L58 54']
  };
  var KIND = {
    brassiere: ['bra','Brassière'], cropped: ['crop','Crop top'], croppedwithneckline: ['crop','Crop col V'], croppedblouse: ['crop','Blouse courte'],
    longsleeveshirt: ['long','Manches longues'], regatta: ['tank','Débardeur'], sweatshirt: ['sweat','Sweat'], tshirts: ['tee','T-shirt'],
    cargopants: ['pants','Cargo'], halfcinnamonpants: ['pants','Pantacourt'], jeans: ['pants','Jean'], rippedpants: ['pants','Jean déchiré'],
    trousers: ['pants','Pantalon'], sweatpants: ['pants','Jogging'], shorts: ['shorts','Short'], panties: ['brief','Culotte'], underwear: ['brief','Caleçon'],
    boot: ['boot','Bottines'], jump: ['sneaker','Montantes'], sneakers: ['sneaker','Baskets'], tennisregular: ['sneaker','Tennis'], tennisshoes: ['sneaker','Tennis sport'],
    slipper: ['slide','Claquettes'], slipperslide: ['slide','Claquettes'], slideslipper: ['slide','Mules'], slideslipperwithsock: ['slide','Mules + chaussettes'], slipperslidewithsock: ['slide','Claquettes + chaussettes'],
    backpack: ['backpack','Sac à dos'], shoulderbag: ['shoulder','Sacoche'], travelbag: ['travel','Sac de voyage'], mask: ['mask','Masque'], ear: ['ear','Boucles d\'oreilles'], clock: ['watch','Montre']
  };
  var KEYKIND = { southTop: 'tee', southBottom: 'pants', southShoes: 'sneaker', southBackpack: 'backpack', southMask: 'mask', southEar: 'ear', southClock: 'watch' };
  var KEYFR = { southTop: 'Hauts', southBottom: 'Bas', southShoes: 'Chaussures', southBackpack: 'Sacs', southMask: 'Masques', southEar: 'Bijoux', southClock: 'Montres' };
  function baseOf(name) { return String(name || '').toLowerCase().replace(/^(lv|versace)_/, '').replace(/[_ ]?\d+$/, ''); }
  function isLux(item) { return /^(lv|versace)_/i.test(item.name || '') || /louisvuitton|versace/.test(item.texture || ''); }
  function kindOf(item, key) { var k = KIND[baseOf(item.name)]; return k ? k[0] : (KEYKIND[key] || 'tee'); }
  function niceName(name, key) { var k = KIND[baseOf(name)], n = String(name || '').match(/(\d+)$/); return (k ? k[1] : pretty(name)) + (n ? ' N°' + n[1] : ''); }
  var maskCss = {};
  (function () {
    var css = '';
    Object.keys(SH).forEach(function (k) {
      var svg = "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><path fill-rule='evenodd' d='" + SH[k][0] + "'/></svg>";
      var u = 'url("data:image/svg+xml,' + encodeURIComponent(svg) + '")';
      css += '.k-' + k + ' .fill,.k-' + k + ' .shade{-webkit-mask-image:' + u + ';mask-image:' + u + '}';
    });
    var st = document.createElement('style'); st.textContent = css; document.head.appendChild(st);
  })();
  function artHTML(kind, big, empty) {
    var hang = /^(tee|long|sweat|tank|crop|pants|shorts)$/.test(kind);
    return '<div class="art k-' + kind + (big ? ' big' : '') + (hang ? ' hang' : '') + '"><i class="fill"' + (empty ? ' style="background:#e6ddd0"' : '') + '></i><i class="shade"></i>' +
      '<svg class="line" viewBox="0 0 100 100" preserveAspectRatio="xMidYMid meet"><path d="' + SH[kind][0] + '"/><path d="' + SH[kind][1] + '"/></svg></div>';
  }
  var lazyObs = null;
  function lazy(el) {
    if (!('IntersectionObserver' in window)) { el.style.backgroundImage = 'url("' + el.dataset.tex + '")'; return; }
    if (!lazyObs) lazyObs = new IntersectionObserver(function (es) { es.forEach(function (e) { if (e.isIntersecting) { e.target.style.backgroundImage = 'url("' + e.target.dataset.tex + '")'; lazyObs.unobserve(e.target); } }); }, { root: $('items'), rootMargin: '120px' });
    lazyObs.observe(el);
  }
  var filter = 'all', filterTab = '';
  function renderFilters(gs) {
    if (filterTab !== active) { filter = 'all'; filterTab = active; }
    var box = $('filters'); box.replaceChildren();
    var kinds = [], lux = false;
    gs.forEach(function (g) { g.items.forEach(function (it) { var k = KIND[baseOf(it.name)], l = k ? k[1] : null; if (l && kinds.indexOf(l) < 0) kinds.push(l); if (isLux(it)) lux = true; }); });
    var chips = [['all', 'Tout']].concat(kinds.slice(0, 8).map(function (l) { return ['k:' + l, l]; }));
    if (lux) chips.push(['lux', '★ Luxe']);
    chips.forEach(function (c) {
      var b = document.createElement('button'); b.textContent = c[1]; b.className = (filter === c[0] ? 'on' : '') + (c[0] === 'lux' ? ' lux' : '');
      b.onclick = function () { filter = c[0]; signature = ''; render(); }; box.appendChild(b);
    });
    box.hidden = chips.length < 3;
  }
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
    $('selection').hidden = $('items').hidden = $('collection-title').hidden = !catalogue; if (!catalogue) $('filters').hidden = true;
    $('fields').style.display = catalogue ? 'none' : 'block';
    $('status').style.display = !state.supported || !state.fields.length ? 'block' : 'none';
    $('status').textContent = !state.fields.length ? 'Chargement de ta garde-robe…' : 'Ce personnage n\'est pas personnalisable. Choisis un autre corps.';
    $('save').disabled = !state.supported || !state.fields.length;
    var nextSignature = active + ':' + focusedKey + ':' + catalogue + ':' + state.model + ':' + JSON.stringify(fields());
    if (nextSignature === signature) return; signature = nextSignature;
    if (catalogue) {
      var chosen = currentGroup(), selected = field(chosen.key).val, piece = chosen.items[selected-1];
      $('piece-name').textContent = piece ? niceName(piece.name, chosen.key) : 'Tenue actuelle';
      $('piece-number').textContent = piece ? 'N°' + String(selected).padStart(3,'0') + (isLux(piece) ? ' · LUXE' : '') : 'ORIGINAL';
      $('piece-count').textContent = chosen.items.length + ' pièces · ' + (KEYFR[chosen.key] || chosen.label);
      $('piece-art').innerHTML = piece ? artHTML(kindOf(piece, chosen.key), true) : artHTML(KEYKIND[chosen.key] || 'tee', true, true);
      if (piece) { var big = $('piece-art').querySelector('.fill'); if (big) big.style.backgroundImage = 'url("south/' + piece.texture + '")'; }
      renderFilters(gs);
      var scroll = $('items').scrollTop; $('items').replaceChildren();
      if (lazyObs) lazyObs.disconnect();
      gs.forEach(function (g) { var f = field(g.key); g.items.forEach(function (item,index) {
        var kind = kindOf(item, g.key), lux = isLux(item);
        if (filter === 'lux' && !lux) return;
        if (filter.indexOf('k:') === 0 && KIND[kind][1] !== filter.slice(2)) return;
        var b = document.createElement('button'); b.className = 'item' + (f.val === index+1 ? ' selected' : ''); b.setAttribute('aria-pressed',String(f.val === index+1));
        b.innerHTML = '<i class="ok"></i>' + artHTML(kind, false);
        var fill = b.querySelector('.fill'); fill.dataset.tex = 'south/' + item.texture; lazy(fill);
        var number = document.createElement('small'); number.textContent = 'N°' + String(index+1).padStart(3,'0'); b.appendChild(number);
        var label = document.createElement('span'); label.textContent = niceName(item.name, g.key); b.appendChild(label);
        var col = document.createElement('em'); col.className = lux ? 'lux' : ''; col.textContent = lux ? 'Édition luxe' : (KEYFR[g.key] || g.label); b.appendChild(col);
        b.title = g.label + ' · ' + label.textContent;
        b.onclick = function () { focusedKey = g.key; setOption(g.key,index+1); }; $('items').appendChild(b);
      }); }); $('items').scrollTop = scroll;
      if (!$('items').children.length) { var e = document.createElement('div'); e.className = 'empty-f'; e.textContent = 'Aucune pièce dans ce filtre.'; $('items').appendChild(e); }
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
