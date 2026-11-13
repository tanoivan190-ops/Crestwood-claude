/* ==========================================================================
   Crestwood RP v20 - langues de l'interface : Francais (source), English, Portugues.
   Choix fait au premier lancement (common/cw-welcome.js), garde dans localStorage "cw.lang"
   (partage par toutes les pages file:// de l'appli : chargement, connexion, HUD).
   - CwI18n.t("Texte francais")       -> texte traduit (ou le francais s'il manque)
   - CwI18n.apply(element)            -> traduit les textes, placeholder, aria-label, title, data-label
   - CwI18n.lang / CwI18n.set("en")   -> langue courante / changer (recharge la page)
   Les messages du serveur (chat, notifications, dialogues) restent ceux du gamemode.
   ========================================================================== */
(function () {
  "use strict";
  var KEY = "cw.lang";
  var lang = "";
  try { lang = window.localStorage.getItem(KEY) || ""; } catch (e) { lang = ""; }
  // [v21] copie native (ecran de chargement) : la langue reste memorisee meme si le stockage web est vide
  if (!/^(fr|en|pt)$/.test(lang)) { try { lang = (window.AndroidLoading && window.AndroidLoading.getPref && window.AndroidLoading.getPref(KEY)) || ""; } catch (e) { lang = ""; } }
  if (!/^(fr|en|pt)$/.test(lang)) lang = "";
  function savePref(k, v) {
    try { window.localStorage.setItem(k, v); } catch (e) { /* stockage indisponible */ }
    try { if (window.AndroidLoading && window.AndroidLoading.setPref) window.AndroidLoading.setPref(k, v); } catch (e) { /* pas de pont */ }
  }

  // "francais": ["english", "portugues"]
  var D = {
    // ---- connexion ----
    "Connexion": ["Log in", "Entrar"],
    "Inscription": ["Sign up", "Cadastro"],
    "Mot de passe": ["Password", "Senha"],
    "Content de te revoir": ["Welcome back", "Bom te ver de novo"],
    "Entre ton mot de passe pour rejoindre la ville.": ["Enter your password to join the city.", "Digite sua senha para entrar na cidade."],
    "Pseudo (Prénom_Nom)": ["Name (First_Last)", "Nome (Nome_Sobrenome)"],
    "Se souvenir de moi": ["Remember me", "Lembrar de mim"],
    "Mot de passe oublié ?": ["Forgot password?", "Esqueceu a senha?"],
    "Se connecter": ["Log in", "Entrar"],
    "Pas encore de compte ?": ["No account yet?", "Ainda sem conta?"],
    "Crée-le en 1 minute": ["Create one in 1 minute", "Crie em 1 minuto"],
    "Créer ton compte": ["Create your account", "Crie sua conta"],
    "Ton pseudo de jeu devient ton identité en ville.": ["Your in-game name becomes your identity in the city.", "Seu nome no jogo vira sua identidade na cidade."],
    "Adresse e-mail": ["Email address", "Endereço de e-mail"],
    "Confirmer": ["Confirm", "Confirmar"],
    "Code promo (facultatif)": ["Promo code (optional)", "Código promocional (opcional)"],
    "Vérifier": ["Check", "Verificar"],
    "J'ai lu et j'accepte le règlement de Crestwood RP": ["I have read and accept the Crestwood RP rules", "Li e aceito as regras do Crestwood RP"],
    "Créer mon compte": ["Create my account", "Criar minha conta"],
    "Demande un code au staff, puis choisis un nouveau mot de passe.": ["Ask the staff for a code, then choose a new password.", "Peça um código à staff e escolha uma nova senha."],
    "Code reçu": ["Code received", "Código recebido"],
    "Demander un code": ["Request a code", "Pedir um código"],
    "Nouveau mot de passe": ["New password", "Nova senha"],
    "Changer le mot de passe": ["Change password", "Trocar a senha"],
    "‹ Retour à la connexion": ["‹ Back to log in", "‹ Voltar ao login"],
    "Ton personnage": ["Your character", "Seu personagem"],
    "Ta vie commence à": ["Your life starts in", "Sua vida começa em"],
    "Une ville vivante": ["A living city", "Uma cidade viva"],
    "Métiers, entreprises, factions et gangs": ["Jobs, businesses, factions and gangs", "Empregos, empresas, facções e gangues"],
    "Économie en dollars": ["Dollar economy", "Economia em dólares"],
    "Banque, garage, maisons et enchères": ["Bank, garage, houses and auctions", "Banco, garagem, casas e leilões"],
    "Code promo à l'inscription": ["Promo code at sign-up", "Código promocional no cadastro"],
    "Véhicule et argent offerts dès ton arrivée": ["Free vehicle and money when you arrive", "Veículo e dinheiro grátis na chegada"],
    "Serveur en ligne": ["Server online", "Servidor online"],
    "Roleplay francophone": ["French-speaking roleplay", "Roleplay em francês"],
    "Accepte le règlement pour créer ton compte.": ["Accept the rules to create your account.", "Aceite as regras para criar sua conta."],
    "Afficher le mot de passe": ["Show password", "Mostrar senha"],
    // ---- chargement / telechargement ----
    "Astuce": ["Tip", "Dica"],
    "Préparation": ["Preparing", "Preparação"],
    "Téléchargement": ["Download", "Download"],
    "Installation": ["Installing", "Instalação"],
    "Vérification": ["Checking", "Verificação"],
    "Installation du jeu": ["Game installation", "Instalação do jogo"],
    "Préparation du jeu": ["Preparing the game", "Preparando o jogo"],
    "Vérification en cours": ["Checking", "Verificando"],
    "Recherche d’une installation existante avant tout téléchargement.": ["Looking for an existing installation before downloading.", "Procurando uma instalação existente antes de baixar."],
    "Téléchargement des fichiers du jeu": ["Downloading game files", "Baixando os arquivos do jogo"],
    "Cette étape n’a lieu qu’une seule fois. Gardez l’application ouverte.": ["This only happens once. Keep the app open.", "Isso só acontece uma vez. Mantenha o app aberto."],
    "Vérification de l’intégrité": ["Checking file integrity", "Verificando a integridade"],
    "Contrôle du fichier téléchargé avant son installation.": ["Checking the downloaded file before installing it.", "Conferindo o arquivo baixado antes de instalar."],
    "Installation des fichiers du jeu": ["Installing game files", "Instalando os arquivos do jogo"],
    "Extraction en cours : ne fermez pas l’application.": ["Extracting: do not close the app.", "Extraindo: não feche o app."],
    "Connexion au serveur": ["Connecting to the server", "Conectando ao servidor"],
    "Chargement de Crestwood Roleplay en cours…": ["Loading Crestwood Roleplay…", "Carregando Crestwood Roleplay…"],
    "Connexion en cours": ["Connecting", "Conectando"],
    "Le jeu s’ouvre dès que le serveur répond.": ["The game opens as soon as the server answers.", "O jogo abre assim que o servidor responder."],
    "Chargement du monde…": ["Loading the world…", "Carregando o mundo…"],
    "Terminé": ["Done", "Concluído"],
    "Données prêtes": ["Data ready", "Dados prontos"],
    "L’installation est terminée. Bon jeu !": ["Installation complete. Have fun!", "Instalação concluída. Bom jogo!"],
    "Installation validée": ["Installation verified", "Instalação validada"],
    "Action requise": ["Action needed", "Ação necessária"],
    "Installation en pause": ["Installation paused", "Instalação pausada"],
    "Réessayer": ["Retry", "Tentar de novo"],
    "Entrer en jeu": ["Enter the game", "Entrar no jogo"],
    "Radio Crestwood": ["Crestwood Radio", "Rádio Crestwood"],
    "En attente des données…": ["Waiting for data…", "Aguardando os dados…"],
    "Téléchargé": ["Downloaded", "Baixado"],
    "Vitesse": ["Speed", "Velocidade"],
    "Temps restant": ["Time left", "Tempo restante"],
    "Connexion instable": ["Unstable connection", "Conexão instável"],
    "Nouvelle tentative automatique en cours…": ["Retrying automatically…", "Tentando de novo automaticamente…"],
    "Autorisez le stockage": ["Allow storage", "Permita o armazenamento"],
    "Reprise de l’installation": ["Resuming installation", "Retomando a instalação"],
    "Pack de données du jeu": ["Game data pack", "Pacote de dados do jogo"],
    // ---- accueil : langue + reglement ----
    "Choisis ta langue": ["Choose your language", "Escolha seu idioma"],
    "Tu pourras la changer plus tard dans les réglages.": ["You can change it later in the settings.", "Você pode mudar depois nas configurações."],
    "Continuer": ["Continue", "Continuar"],
    "Règlement de Crestwood": ["Crestwood rules", "Regras de Crestwood"],
    "Lis les règles de la ville avant d'entrer. Elles s'appliquent à tous les joueurs.": ["Read the city rules before entering. They apply to every player.", "Leia as regras da cidade antes de entrar. Elas valem para todos os jogadores."],
    "J'ai lu et j'accepte le règlement": ["I have read and accept the rules", "Li e aceito as regras"],
    "Accepter et entrer": ["Accept and enter", "Aceitar e entrar"],
    "Langue": ["Language", "Idioma"],
    // ---- HUD ----
    "Menu": ["Menu", "Menu"],
    "Réglages": ["Settings", "Ajustes"],
    "Téléphone": ["Phone", "Celular"],
    "Moi": ["Me", "Eu"],
    "Ville": ["City", "Cidade"],
    "Véhicules": ["Vehicles", "Veículos"],
    "Travail": ["Work", "Trabalho"],
    "Fermer": ["Close", "Fechar"],
    "Mes papiers": ["My papers", "Meus documentos"],
    "Inventaire": ["Inventory", "Inventário"],
    "Vêtements": ["Clothes", "Roupas"],
    "Animations": ["Animations", "Animações"],
    "Faim et soif": ["Hunger and thirst", "Fome e sede"],
    "Mon niveau": ["My level", "Meu nível"],
    "Tablette RP": ["RP tablet", "Tablet RP"],
    "Support": ["Support", "Suporte"],
    "Aide": ["Help", "Ajuda"],
    "Carte": ["Map", "Mapa"],
    "Boutique": ["Store", "Loja"],
    "Quêtes": ["Quests", "Missões"],
    "Loto": ["Lottery", "Loteria"],
    "GPS mairie": ["City hall GPS", "GPS prefeitura"],
    "Mon garage": ["My garage", "Minha garagem"],
    "Commandes du véhicule": ["Vehicle controls", "Comandos do veículo"],
    "Ceinture": ["Seat belt", "Cinto"],
    "Rendre la location": ["Return rental", "Devolver aluguel"],
    "Course de rue": ["Street race", "Racha"],
    "Emplois": ["Jobs", "Empregos"],
    "Casier": ["Records", "Ficha"],
    "Minimap": ["Minimap", "Minimapa"],
    "Chat": ["Chat", "Chat"],
    "Affiché": ["Shown", "Visível"],
    "Masqué": ["Hidden", "Oculto"],
    "Premiers pas": ["First steps", "Primeiros passos"],
    "Tes premiers objectifs": ["Your first goals", "Seus primeiros objetivos"],
    "Recommandé": ["Recommended", "Recomendado"],
    "Loue un deux-roues": ["Rent a two-wheeler", "Alugue uma moto ou bike"],
    "Fais tes papiers": ["Get your ID", "Tire seus documentos"],
    "Ouvre un compte en banque": ["Open a bank account", "Abra uma conta no banco"],
    "Trouve un emploi": ["Find a job", "Arrume um emprego"],
    "Envoyer": ["Send", "Enviar"],
    "espace": ["space", "espaço"],
    "Phrases": ["Phrases", "Frases"],
    "Caméra": ["Camera", "Câmera"],
    "Ambiance": ["Scene", "Cena"],
    "Selfie": ["Selfie", "Selfie"],
    "Retour": ["Back", "Voltar"],
    "Grille": ["Grid", "Grade"],
    "Minuteur": ["Timer", "Timer"],
    "Format": ["Format", "Formato"],
    "Photos": ["Photos", "Fotos"],
    "Prendre la photo": ["Take photo", "Tirar foto"],
    "Capture en cours...": ["Capturing...", "Capturando..."],
    "Touche pour annuler": ["Tap to cancel", "Toque para cancelar"],
    "Interagir": ["Interact", "Interagir"],
    "Aide et commandes": ["Help and commands", "Ajuda e comandos"],
    "Blazer (factions)": ["Blazer (factions)", "Blazer (facções)"],
    "Carte de la ville": ["City map", "Mapa da cidade"],
    "Contacter le staff": ["Contact staff", "Falar com a staff"],
    "Fil du chat": ["Chat feed", "Chat"],
    "Loto de Crestwood": ["Crestwood lottery", "Loteria de Crestwood"],
    "Mon emploi": ["My job", "Meu emprego"],
    "Niveau et XP": ["Level and XP", "Nível e XP"],
    "Papiers d'identité": ["ID papers", "Documentos"],
    "Paramètres": ["Settings", "Configurações"],
    "Sac": ["Bag", "Mochila"],
    "Tablette job": ["Job tablet", "Tablet de emprego"],
    "Tenue": ["Outfit", "Roupa"],
    "Mon personnage": ["My character", "Meu personagem"],
    "Contacts et aide": ["Contacts and help", "Contatos e ajuda"],
    "Se déplacer, acheter": ["Get around, shop", "Circular, comprar"],
    "Garage et conduite": ["Garage and driving", "Garagem e direção"],
    "Emplois et factions": ["Jobs and factions", "Empregos e facções"],
    "Affichage du jeu": ["Game display", "Exibição do jogo"],
    "Drone": ["Drone", "Drone"],
    "Photo": ["Photo", "Foto"],
    "Me poser": ["Land here", "Pousar aqui"],
    "maintenant": ["now", "agora"],
    "Touche pour ouvrir": ["Tap to open", "Toque para abrir"],
    "Virement reçu": ["Transfer received", "Transferência recebida"]
  };
  var IDX = lang === "en" ? 0 : (lang === "pt" ? 1 : -1);

  function t(a, b) {
    var fr = (b !== undefined && b !== null) ? b : a;
    if (IDX < 0) return fr;
    var e = D[fr];
    return e ? e[IDX] : fr;
  }
  function trText(node) {
    var raw = node.nodeValue;
    var s = raw.trim();
    if (!s) return;
    var e = D[s];
    if (e) node.nodeValue = raw.replace(s, e[IDX]);
  }
  var ATTRS = ["placeholder", "aria-label", "title", "data-label"];
  function apply(root) {
    if (IDX < 0 || !root) return;
    var walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT, null);
    var n;
    while ((n = walker.nextNode())) {
      var p = n.parentNode && n.parentNode.nodeName;
      if (p === "SCRIPT" || p === "STYLE") continue;
      trText(n);
    }
    var els = root.querySelectorAll ? root.querySelectorAll("[placeholder],[aria-label],[title],[data-label]") : [];
    for (var i = 0; i < els.length; i++) {
      for (var k = 0; k < ATTRS.length; k++) {
        var v = els[i].getAttribute(ATTRS[k]);
        if (v && D[v.trim()]) els[i].setAttribute(ATTRS[k], D[v.trim()][IDX]);
      }
    }
    if (document.documentElement) document.documentElement.lang = lang || "fr";
  }
  function set(code) {
    if (!/^(fr|en|pt)$/.test(code)) return;
    savePref(KEY, code);
    window.location.reload();
  }
  /** Premier choix (accueil) : applique la langue a la page deja affichee, sans la recharger. */
  function live(code) {
    if (!/^(fr|en|pt)$/.test(code)) return;
    lang = code;
    IDX = lang === "en" ? 0 : (lang === "pt" ? 1 : -1);
    api.lang = lang; api.chosen = true;
    savePref(KEY, code);
    apply(document.body);
    try { window.dispatchEvent(new Event("cw:lang")); } catch (e) { /* ancien moteur */ }
  }
  var api = { t: t, apply: apply, set: set, live: live, lang: lang || "fr", chosen: !!lang, dict: D, savePref: savePref };
  window.CwI18n = api;
})();
