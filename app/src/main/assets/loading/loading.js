(()=>{
  'use strict';
  const $=id=>document.getElementById(id);
  const T=s=>(window.CwI18n?window.CwI18n.t(s):s);   // [v20] langue choisie au premier lancement
  const dlBox=$('dl'),dlDone=$('dlDone'),dlTotal=$('dlTotal'),dlSpeed=$('dlSpeed'),dlEta=$('dlEta'),spark=$('dlSpark');
  const speedHist=[];
  const card=$('card'),bar=$('bar'),percentEl=$('percent'),bytesEl=$('bytes'),speedEl=$('speed'),etaEl=$('eta');
  const title=$('title'),message=$('message'),stage=$('stage'),status=$('status'),detail=$('detail'),retry=$('retry'),skip=$('skip');
  const track=document.querySelector('.track');
  const steps=Array.prototype.slice.call(document.querySelectorAll('#steps li'));

  let sawDownload=false,smoothSpeed=0,lastDone=-1,lastTime=0,skipTimer=0;
  const SKIP_DELAY_MS=6000; // le bouton "Entrer en jeu" apparaît 6 s après le début de la connexion

  // ---------- Lecteur de musique (pistes dans music/) ----------
  const TRACKS=[
    {src:'music/track1.mp3',title:'SPRINTER - CENTRAL CEE x DAVE'},
    {src:'music/track2.mp3',title:'PILIER - NINHO'},
    {src:'music/track3.mp3',title:'PACMAN DE PANICAT - MC RYAN SP, MC POZE DO RODO, RODRIGO DO CN'},
    {src:'music/track4.mp3',title:'PACMAN DE PANICAT, EU PREFIRO A RUA'},
    {src:'music/track5.mp3',title:'PACMAN DE PANICAT - BASE AUTOMOTIVA'},
    {src:'music/track6.mp3',title:'MERIDIAN - TIAKOLA x DAVE'}
  ];
  const audio=new Audio();
  audio.preload='auto';
  const titleEl=$('trackTitle'),playBtn=$('play'),playIcon=$('playIcon'),prevBtn=$('prev'),nextBtn=$('next'),vol=$('vol');
  const ICON_PAUSE='M7 5h4v14H7zM13 5h4v14h-4z',ICON_PLAY='M8 5v14l11-7z';
  let trackIndex=Math.floor(Math.random()*TRACKS.length),errors=0,stopped=false;

  function showIcon(playing){
    playIcon.firstElementChild.setAttribute('d',playing?ICON_PAUSE:ICON_PLAY);
    playBtn.setAttribute('aria-label',playing?'Pause':'Lecture');
  }
  function loadTrack(i){
    trackIndex=(i+TRACKS.length)%TRACKS.length;
    audio.src=TRACKS[trackIndex].src;
    titleEl.textContent=TRACKS[trackIndex].title;
  }
  function playAudio(){
    if(stopped)return;
    const p=audio.play();
    if(p&&typeof p.catch==='function')p.catch(()=>showIcon(false));
  }
  function setVolume(v){
    v=Math.max(0,Math.min(100,Number(v)||0));
    audio.volume=v/100;
    vol.style.setProperty('--v',v+'%');
  }
  function stopMusic(){
    stopped=true;
    try{audio.pause();audio.removeAttribute('src');audio.load();}catch(e){}
  }
  audio.addEventListener('play',()=>showIcon(true));
  audio.addEventListener('pause',()=>showIcon(false));
  audio.addEventListener('playing',()=>{errors=0;});
  audio.addEventListener('ended',()=>{loadTrack(trackIndex+1);playAudio();});
  audio.addEventListener('error',()=>{
    if(stopped||++errors>TRACKS.length)return;
    window.setTimeout(()=>{loadTrack(trackIndex+1);playAudio();},300);
  });
  playBtn.addEventListener('click',()=>{if(audio.paused)playAudio();else audio.pause();});
  nextBtn.addEventListener('click',()=>{loadTrack(trackIndex+1);playAudio();});
  prevBtn.addEventListener('click',()=>{
    if(audio.currentTime>3){audio.currentTime=0;return;}
    loadTrack(trackIndex-1);playAudio();
  });
  vol.addEventListener('input',()=>setVolume(vol.value));
  setVolume(vol.value);
  loadTrack(trackIndex);
  playAudio();

  const clamp=v=>Math.max(0,Math.min(100,Number(v)||0));
  const norm=t=>String(t||'').normalize('NFD').replace(/[\u0300-\u036f]/g,'').toLowerCase();
  const dec=(v,d)=>v.toFixed(d).replace('.',',');
  const U=()=>((window.CwI18n&&window.CwI18n.lang!=='fr')?['KB','MB','GB']:['Ko','Mo','Go']);
  const fmtBytes=v=>{
    v=Math.max(0,Number(v)||0);
    const u=U();
    if(v<1048576)return Math.round(v/1024)+' '+u[0];
    if(v<1073741824)return dec(v/1048576,1)+' '+u[1];
    return dec(v/1073741824,2)+' '+u[2];
  };
  const fmtTime=s=>{
    s=Math.max(0,Math.round(s));
    if(s<60)return s+' s';
    const m=Math.floor(s/60);
    if(m<60)return m+' min '+String(s%60).padStart(2,'0')+' s';
    return Math.floor(m/60)+' h '+String(m%60).padStart(2,'0')+' min';
  };

  // Textes affichés pour chaque étape (la phrase exacte envoyée par l'application s'affiche dans #message).
  const INFO={
    prepare:{state:'idle',stage:'Préparation',title:'Installation du jeu',status:'Vérification en cours',
      detail:'Recherche d’une installation existante avant tout téléchargement.'},
    download:{state:'download',stage:'Téléchargement',title:'Installation du jeu',status:'Téléchargement des fichiers du jeu',
      detail:'Cette étape n’a lieu qu’une seule fois. Gardez l’application ouverte.'},
    verify:{state:'verify',stage:'Vérification',title:'Installation du jeu',status:'Vérification de l’intégrité',
      detail:'Contrôle du fichier téléchargé avant son installation.'},
    install:{state:'install',stage:'Installation',title:'Installation du jeu',status:'Installation des fichiers du jeu',
      detail:'Extraction en cours : ne fermez pas l’application.'}
  };

  function setSteps(current,allDone){
    steps.forEach((li,i)=>{
      const n=i+1;
      li.className=allDone||n<current?'done':n===current?'active':'';
    });
  }
  function setBar(pct){
    bar.style.width=pct+'%';
    percentEl.textContent=Math.round(pct)+' %';
    track.setAttribute('aria-valuenow',String(Math.round(pct)));
  }

  function setProgress(value,phase,done,total){
    const pct=clamp(value),p=norm(phase),now=Date.now();
    done=Math.max(0,Number(done)||0);
    total=Math.max(0,Number(total)||0);

    let key='prepare',step=1;
    if(p==='donnees installees'||pct>=100){key='install';step=4;}
    else if(/installation des fichiers|extraits|finalisation/.test(p)){key='install';step=3;}
    else if(/integrite/.test(p)){key='verify';step=2;}
    else if(/^telechargement|reprise/.test(p)){key='download';step=2;}
    const info=INFO[key];

    // Vitesse lissée (calculée ici, indépendamment de l'application)
    if(key==='download'){
      sawDownload=true;
      if(lastDone>=0&&done<lastDone)smoothSpeed=0;
      if(lastDone>=0&&done>lastDone&&now>lastTime){
        const instant=(done-lastDone)*1000/(now-lastTime);
        smoothSpeed=smoothSpeed?smoothSpeed*0.8+instant*0.2:instant;
      }
      lastDone=done;lastTime=now;
    }else{
      smoothSpeed=0;lastDone=-1;
    }

    stopBusy();
    card.setAttribute('data-state',info.state);
    stage.textContent=T(info.stage);
    title.textContent=T(info.title);
    message.textContent=phase?T(phase):T(info.title);
    if(/nouvelle tentative/.test(p)){
      status.textContent=T('Connexion instable');
      detail.textContent=T('Nouvelle tentative automatique en cours…');
    }else{
      status.textContent=T(info.status);
      detail.textContent=T(info.detail);
    }
    setSteps(step,pct>=100);
    setBar(pct);
    bytesEl.textContent=total>0?fmtBytes(done)+' / '+fmtBytes(total):T('En attente des données…');
    speedEl.textContent=key==='download'&&smoothSpeed>0?T('Vitesse')+' : '+fmtBytes(smoothSpeed)+'/s':'';
    etaEl.textContent=key==='download'&&smoothSpeed>0&&total>done?T('Temps restant')+' : ~'+fmtTime((total-done)/smoothSpeed):'';
    renderDl(key,done,total);
    retry.hidden=true;
  }

  // [v20] tableau de telechargement + courbe de vitesse (60 derniers points)
  function drawSpark(){
    if(!spark||!spark.getContext)return;
    const c=spark.getContext('2d'),w=spark.width,h=spark.height;
    c.clearRect(0,0,w,h);
    if(speedHist.length<2)return;
    const max=Math.max.apply(null,speedHist)||1,n=speedHist.length,step=w/(60-1);
    c.beginPath();
    speedHist.forEach((v,i)=>{const x=w-(n-1-i)*step,y=h-2-(v/max)*(h-6);if(i===0)c.moveTo(x,y);else c.lineTo(x,y);});
    c.lineWidth=2;c.strokeStyle='#3b82ff';c.stroke();
    c.lineTo(w,h);c.lineTo(w-(n-1)*step,h);c.closePath();c.fillStyle='rgba(59,130,255,.18)';c.fill();
  }
  function renderDl(key,done,total){
    const on=key==='download'||key==='verify'||key==='install';
    if(dlBox)dlBox.hidden=!on;
    document.body.classList.toggle('dl-on',on);
    if(!on)return;
    dlDone.textContent=fmtBytes(done);
    dlTotal.textContent=total>0?'/ '+fmtBytes(total):'\u00a0';
    if(key==='download'&&smoothSpeed>0){
      dlSpeed.textContent=fmtBytes(smoothSpeed)+'/s';
      speedHist.push(smoothSpeed);if(speedHist.length>60)speedHist.shift();
      dlEta.textContent=total>done?fmtTime((total-done)/smoothSpeed):'—';
    }else{
      dlSpeed.textContent='—';
      dlEta.textContent=key==='download'?'…':'—';
    }
    drawSpark();
  }

  // Arrête l'animation "connexion" et cache le bouton "Entrer en jeu"
  function stopBusy(){
    track.classList.remove('busy');
    window.clearTimeout(skipTimer);
    skip.hidden=true;
  }

  function connecting(){
    renderDl('none',0,0);
    card.setAttribute('data-state','idle');
    setSteps(4,true);
    stage.textContent=T('Connexion');
    title.textContent=T('Connexion au serveur');
    message.textContent=T('Chargement de Crestwood Roleplay en cours…');
    status.textContent=T('Connexion en cours');
    detail.textContent=T('Le jeu s’ouvre dès que le serveur répond.');
    bytesEl.textContent=T('Chargement du monde…');
    percentEl.textContent=T('…');
    track.classList.add('busy');
    speedEl.textContent=T('');
    etaEl.textContent=T('');
    retry.hidden=true;
    // Si la connexion traîne, le joueur peut fermer l'écran lui-même.
    window.clearTimeout(skipTimer);
    skipTimer=window.setTimeout(()=>{skip.hidden=false;},SKIP_DELAY_MS);
  }

  function ready(){
    renderDl('none',0,0);
    stopBusy();
    card.setAttribute('data-state','ready');
    setSteps(4,true);
    setBar(100);
    stage.textContent=T('Terminé');
    title.textContent=T('Données prêtes');
    message.textContent=T(sawDownload?'L’installation est terminée. Bon jeu !':'Vos données sont déjà installées : aucun téléchargement n’a été nécessaire.');
    status.textContent=T('Installation validée');
    detail.textContent=T(sawDownload?'Téléchargement, vérification et installation terminés.':'Les fichiers du jeu ont été trouvés sur cet appareil.');
    speedEl.textContent=T('');
    etaEl.textContent=T('');
    retry.hidden=true;
  }

  function error(text){
    renderDl('none',0,0);
    stopBusy();
    card.setAttribute('data-state','error');
    stage.textContent=T('Action requise');
    title.textContent=T('Installation en pause');
    message.textContent=text||T('Impossible de terminer l’installation des données.');
    status.textContent=T('L’installation doit être reprise');
    detail.textContent=T('Le jeu reste bloqué tant que les fichiers obligatoires ne sont pas installés.');
    speedEl.textContent=T('');
    etaEl.textContent=T('Appuyez sur RÉESSAYER pour reprendre.');
    retry.hidden=false;
  }

  function storageAccessRequired(){
    renderDl('none',0,0);
    stopBusy();
    card.setAttribute('data-state','permission');
    setSteps(1,false);
    stage.textContent=T('Autorisation requise');
    title.textContent=T('Autorisez le stockage');
    message.textContent=T('Autorisez l’accès aux fichiers pour installer les données dans /storage/emulated/0/data.');
    status.textContent=T('En attente de l’autorisation');
    detail.textContent=T('Dès que l’accès est accordé, l’installation démarre automatiquement.');
    speedEl.textContent=T('');
    etaEl.textContent=T('');
    retry.hidden=true;
  }

  function storageAccessGranted(){
    card.setAttribute('data-state','idle');
    title.textContent=T('Préparation du jeu');
    message.textContent=T('Accès autorisé. Démarrage de l’installation…');
    status.textContent=T('Préparation du téléchargement');
    detail.textContent=T('Les fichiers seront installés dans /storage/emulated/0/data.');
  }

  retry.addEventListener('click',()=>{
    retry.hidden=true;
    card.setAttribute('data-state','idle');
    setSteps(1,false);
    stage.textContent=T('Reprise');
    title.textContent=T('Reprise de l’installation');
    message.textContent=T('Vérification des fichiers déjà téléchargés…');
    status.textContent=T('Préparation de la reprise');
    detail.textContent=T('Un fichier valide déjà téléchargé sera réutilisé.');
    etaEl.textContent=T('');
    if(window.AndroidLoading&&typeof window.AndroidLoading.retry==='function')window.AndroidLoading.retry();
  });

  skip.addEventListener('click',()=>{
    skip.hidden=true;
    if(window.AndroidLoading&&typeof window.AndroidLoading.skip==='function')window.AndroidLoading.skip();
  });

  // ---------- [v19] diaporama + astuces RP ----------
  const slides=Array.prototype.slice.call(document.querySelectorAll('#slides .slide'));
  let slideIndex=0;
  if(slides.length>1)window.setInterval(()=>{
    slides[slideIndex].classList.remove('on');
    slideIndex=(slideIndex+1)%slides.length;
    const s=slides[slideIndex];
    s.style.animation='none';void s.offsetWidth;s.style.animation='';   // relance le zoom lent
    s.classList.add('on');
  },7000);
  // [v20] plus d'affiche avec logo integre : le nouveau logo PNG reste affiche en haut
  const TIPS=[
    'Utilise /me et /do pour décrire tes actions : c’est la base du roleplay.',
    'Nouveau ? Entre un code promo à l’inscription pour recevoir un véhicule et de l’argent.',
    'Mange et bois régulièrement : la faim et la soif s’affichent autour de ta carte.',
    'Chaque heure de jeu te rapporte de l’expérience. Tape /niveau pour voir ta progression.',
    'Décris ton personnage avec /apparence : les autres le découvrent avec /examiner.',
    'Tente ta chance au loto de Crestwood dans les superettes 24/7 : /loto.',
    'Ta carte d’identité se fait à l’état civil. Sans elle, /papiers t’y guide au GPS.',
    'Tes véhicules t’attendent partout : /garage ou la roue du HUD.',
    'Besoin d’aide ? /report contacte le staff, /aide liste les commandes.',
    'Les zones vertes interdisent toute violence : respecte-les.'
  ];
  const TIPS_I18N={
    en:['Use /me and /do to describe your actions: the basis of roleplay.','New? Enter a promo code at sign-up to get a vehicle and money.',
      'Eat and drink regularly: hunger and thirst are shown around your map.','Every hour played gives you XP. Type /niveau to see your progress.',
      'Describe your character with /apparence: others see it with /examiner.','Try the Crestwood lottery in 24/7 stores: /loto.',
      'Your ID card is made at the civil registry. Without it, /papiers guides you there.','Your vehicles wait for you everywhere: /garage or the HUD wheel.',
      'Need help? /report reaches the staff, /aide lists the commands.','Green zones forbid any violence: respect them.'],
    pt:['Use /me e /do para descrever suas ações: a base do roleplay.','Novo? Digite um código promocional no cadastro para ganhar veículo e dinheiro.',
      'Coma e beba sempre: fome e sede aparecem em volta do mapa.','Cada hora de jogo dá XP. Digite /niveau para ver seu progresso.',
      'Descreva seu personagem com /apparence: os outros veem com /examiner.','Tente a loteria de Crestwood nas lojas 24/7: /loto.',
      'Sua identidade é feita no cartório. Sem ela, /papiers te guia até lá.','Seus veículos te esperam em todo lugar: /garage ou a roda do HUD.',
      'Precisa de ajuda? /report chama a staff, /aide lista os comandos.','Zonas verdes proíbem qualquer violência: respeite.']
  };
  const L=window.CwI18n&&window.CwI18n.lang;
  if(TIPS_I18N[L])TIPS.splice(0,TIPS.length,...TIPS_I18N[L]);
  window.addEventListener('cw:lang',()=>{const l=window.CwI18n.lang;if(TIPS_I18N[l])TIPS.splice(0,TIPS.length,...TIPS_I18N[l]);});
  const tipBox=$('tip'),tipText=$('tipText');
  let tipIndex=Math.floor(Math.random()*TIPS.length);
  if(tipText){
    tipText.textContent=TIPS[tipIndex];
    window.setInterval(()=>{
      tipBox.classList.add('out');
      window.setTimeout(()=>{tipIndex=(tipIndex+1)%TIPS.length;tipText.textContent=TIPS[tipIndex];tipBox.classList.remove('out');},360);
    },6500);
  }

  window.LoadingBridge={setProgress,ready,error,storageAccessRequired,storageAccessGranted,stopMusic,connecting};
  setSteps(1,false);
  if(window.AndroidLoading&&typeof window.AndroidLoading.start==='function')window.AndroidLoading.start();
})();
