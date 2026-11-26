.class public Lcom/gta/game/ui/LoadingScreen;
.super Ljava/lang/Object;
.source "LoadingScreen.java"


# annotations
.annotation system Ldalvik/annotation/MemberClasses;
    value = {
        Lcom/gta/game/ui/LoadingScreen$LoadingBridge;
    }
.end annotation


# static fields
.field private static final FADE_MS:J = 0x15eL

.field private static final MAX_WAIT_MS:J = 0x7530L


# instance fields
.field private final activity:Landroid/app/Activity;

.field private completed:Z

.field private handler:Landroid/os/Handler;

.field private hiding:Z

.field private loadingWebView:Landroid/webkit/WebView;

.field private mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;


# direct methods
.method public static synthetic $r8$lambda$OWbVL-N-id6vNEenb9vMIHwnxTE(Lcom/gta/game/ui/LoadingScreen;)V
    .locals 0

    invoke-direct {p0}, Lcom/gta/game/ui/LoadingScreen;->lambda$hide$0()V

    return-void
.end method

.method public static synthetic $r8$lambda$X-gU-ms_NHq5TnflUA1AY4_GUkQ(Lcom/gta/game/ui/LoadingScreen;)V
    .locals 0

    invoke-direct {p0}, Lcom/gta/game/ui/LoadingScreen;->lambda$show$1()V

    return-void
.end method

.method static bridge synthetic -$$Nest$fgetactivity(Lcom/gta/game/ui/LoadingScreen;)Landroid/app/Activity;
    .locals 0

    iget-object p0, p0, Lcom/gta/game/ui/LoadingScreen;->activity:Landroid/app/Activity;

    return-object p0
.end method

.method static bridge synthetic -$$Nest$fgetcompleted(Lcom/gta/game/ui/LoadingScreen;)Z
    .locals 0

    iget-boolean p0, p0, Lcom/gta/game/ui/LoadingScreen;->completed:Z

    return p0
.end method

.method static bridge synthetic -$$Nest$fgethandler(Lcom/gta/game/ui/LoadingScreen;)Landroid/os/Handler;
    .locals 0

    iget-object p0, p0, Lcom/gta/game/ui/LoadingScreen;->handler:Landroid/os/Handler;

    return-object p0
.end method

.method static bridge synthetic -$$Nest$fgetloadingWebView(Lcom/gta/game/ui/LoadingScreen;)Landroid/webkit/WebView;
    .locals 0

    iget-object p0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    return-object p0
.end method

.method static bridge synthetic -$$Nest$fgetmainLayout(Lcom/gta/game/ui/LoadingScreen;)Landroidx/constraintlayout/widget/ConstraintLayout;
    .locals 0

    iget-object p0, p0, Lcom/gta/game/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    return-object p0
.end method

.method static bridge synthetic -$$Nest$fputcompleted(Lcom/gta/game/ui/LoadingScreen;Z)V
    .locals 0

    iput-boolean p1, p0, Lcom/gta/game/ui/LoadingScreen;->completed:Z

    return-void
.end method

.method static bridge synthetic -$$Nest$fputloadingWebView(Lcom/gta/game/ui/LoadingScreen;Landroid/webkit/WebView;)V
    .locals 0

    iput-object p1, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    return-void
.end method

.method static bridge synthetic -$$Nest$mevaluate(Lcom/gta/game/ui/LoadingScreen;Ljava/lang/String;)V
    .locals 0

    invoke-direct {p0, p1}, Lcom/gta/game/ui/LoadingScreen;->evaluate(Ljava/lang/String;)V

    return-void
.end method

.method static bridge synthetic -$$Nest$mrunOnUiThread(Lcom/gta/game/ui/LoadingScreen;Ljava/lang/Runnable;)V
    .locals 0

    invoke-direct {p0, p1}, Lcom/gta/game/ui/LoadingScreen;->runOnUiThread(Ljava/lang/Runnable;)V

    return-void
.end method

.method static bridge synthetic -$$Nest$smquote(Ljava/lang/String;)Ljava/lang/String;
    .locals 0

    invoke-static {p0}, Lcom/gta/game/ui/LoadingScreen;->quote(Ljava/lang/String;)Ljava/lang/String;

    move-result-object p0

    return-object p0
.end method

.method public constructor <init>(Landroid/app/Activity;)V
    .locals 3
    .param p1, "activity"    # Landroid/app/Activity;

    .line 28
    invoke-direct {p0}, Ljava/lang/Object;-><init>()V

    .line 29
    iput-object p1, p0, Lcom/gta/game/ui/LoadingScreen;->activity:Landroid/app/Activity;

    .line 30
    invoke-virtual {p1}, Landroid/app/Activity;->getLayoutInflater()Landroid/view/LayoutInflater;

    move-result-object v0

    sget v1, Lcom/gta/game/R$layout;->loadingscreen:I

    const/4 v2, 0x0

    invoke-virtual {v0, v1, v2}, Landroid/view/LayoutInflater;->inflate(ILandroid/view/ViewGroup;)Landroid/view/View;

    move-result-object v0

    check-cast v0, Landroidx/constraintlayout/widget/ConstraintLayout;

    iput-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    .line 31
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    new-instance v1, Landroidx/constraintlayout/widget/ConstraintLayout$LayoutParams;

    const/4 v2, -0x1

    invoke-direct {v1, v2, v2}, Landroidx/constraintlayout/widget/ConstraintLayout$LayoutParams;-><init>(II)V

    invoke-virtual {p1, v0, v1}, Landroid/app/Activity;->addContentView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 34
    invoke-direct {p0}, Lcom/gta/game/ui/LoadingScreen;->initializeViews()V

    .line 35
    return-void
.end method

.method private evaluate(Ljava/lang/String;)V
    .locals 2
    .param p1, "script"    # Ljava/lang/String;

    .line 106
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    if-eqz v0, :cond_0

    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    if-eqz v0, :cond_0

    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    invoke-virtual {v0}, Landroidx/constraintlayout/widget/ConstraintLayout;->getVisibility()I

    move-result v0

    if-nez v0, :cond_0

    .line 107
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    const/4 v1, 0x0

    invoke-virtual {v0, p1, v1}, Landroid/webkit/WebView;->evaluateJavascript(Ljava/lang/String;Landroid/webkit/ValueCallback;)V

    .line 109
    :cond_0
    return-void
.end method

.method private initializeViews()V
    .locals 4

    .line 38
    new-instance v0, Landroid/os/Handler;

    invoke-static {}, Landroid/os/Looper;->getMainLooper()Landroid/os/Looper;

    move-result-object v1

    invoke-direct {v0, v1}, Landroid/os/Handler;-><init>(Landroid/os/Looper;)V

    iput-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->handler:Landroid/os/Handler;

    .line 39
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    sget v1, Lcom/gta/game/R$id;->loadingWebView:I

    invoke-virtual {v0, v1}, Landroidx/constraintlayout/widget/ConstraintLayout;->findViewById(I)Landroid/view/View;

    move-result-object v0

    check-cast v0, Landroid/webkit/WebView;

    iput-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    .line 40
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    if-nez v0, :cond_0

    return-void

    .line 42
    :cond_0
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    const v1, -0xfafafa

    invoke-virtual {v0, v1}, Landroid/webkit/WebView;->setBackgroundColor(I)V

    .line 43
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    new-instance v1, Lcom/gta/game/ui/LoadingScreen$1;

    invoke-direct {v1, p0}, Lcom/gta/game/ui/LoadingScreen$1;-><init>(Lcom/gta/game/ui/LoadingScreen;)V

    invoke-virtual {v0, v1}, Landroid/webkit/WebView;->setWebViewClient(Landroid/webkit/WebViewClient;)V

    .line 64
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    invoke-virtual {v0}, Landroid/webkit/WebView;->getSettings()Landroid/webkit/WebSettings;

    move-result-object v0

    .line 65
    .local v0, "settings":Landroid/webkit/WebSettings;
    const/4 v1, 0x1

    invoke-virtual {v0, v1}, Landroid/webkit/WebSettings;->setJavaScriptEnabled(Z)V

    .line 66
    const/4 v2, 0x0

    invoke-virtual {v0, v2}, Landroid/webkit/WebSettings;->setDomStorageEnabled(Z)V

    .line 67
    invoke-virtual {v0, v1}, Landroid/webkit/WebSettings;->setAllowFileAccess(Z)V

    .line 68
    invoke-virtual {v0, v2}, Landroid/webkit/WebSettings;->setAllowContentAccess(Z)V

    .line 69
    invoke-virtual {v0, v2}, Landroid/webkit/WebSettings;->setBuiltInZoomControls(Z)V

    .line 70
    invoke-virtual {v0, v2}, Landroid/webkit/WebSettings;->setDisplayZoomControls(Z)V

    .line 71
    invoke-virtual {v0, v1}, Landroid/webkit/WebSettings;->setLoadWithOverviewMode(Z)V

    .line 72
    invoke-virtual {v0, v1}, Landroid/webkit/WebSettings;->setUseWideViewPort(Z)V

    .line 73
    invoke-virtual {v0, v2}, Landroid/webkit/WebSettings;->setMediaPlaybackRequiresUserGesture(Z)V

    .line 74
    iget-object v1, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    new-instance v2, Lcom/gta/game/ui/LoadingScreen$LoadingBridge;

    const/4 v3, 0x0

    invoke-direct {v2, p0, v3}, Lcom/gta/game/ui/LoadingScreen$LoadingBridge;-><init>(Lcom/gta/game/ui/LoadingScreen;Lcom/gta/game/ui/LoadingScreen-IA;)V

    const-string v3, "AndroidLoading"

    invoke-virtual {v1, v2, v3}, Landroid/webkit/WebView;->addJavascriptInterface(Ljava/lang/Object;Ljava/lang/String;)V

    .line 75
    iget-object v1, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    const-string v2, "file:///android_asset/loading/index.html"

    invoke-virtual {v1, v2}, Landroid/webkit/WebView;->loadUrl(Ljava/lang/String;)V

    .line 77
    new-instance v1, Lcom/gta/game/ui/LoadingScreen$2;

    invoke-direct {v1, p0}, Lcom/gta/game/ui/LoadingScreen$2;-><init>(Lcom/gta/game/ui/LoadingScreen;)V

    invoke-static {v1}, Lcom/rockstargames/oswrapper/DataInstaller;->setListener(Lcom/rockstargames/oswrapper/DataInstaller$Listener;)V

    .line 97
    iget-object v1, p0, Lcom/gta/game/ui/LoadingScreen;->activity:Landroid/app/Activity;

    invoke-static {v1}, Lcom/rockstargames/oswrapper/DataInstaller;->prepare(Landroid/content/Context;)V

    .line 98
    return-void
.end method

.method private synthetic lambda$hide$0()V
    .locals 2

    .line 124
    const/4 v0, 0x0

    iput-boolean v0, p0, Lcom/gta/game/ui/LoadingScreen;->hiding:Z

    .line 125
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    if-eqz v0, :cond_0

    .line 126
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    const/16 v1, 0x8

    invoke-virtual {v0, v1}, Landroidx/constraintlayout/widget/ConstraintLayout;->setVisibility(I)V

    .line 127
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    const/high16 v1, 0x3f800000    # 1.0f

    invoke-virtual {v0, v1}, Landroidx/constraintlayout/widget/ConstraintLayout;->setAlpha(F)V

    .line 130
    :cond_0
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    if-eqz v0, :cond_1

    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    const-string v1, "about:blank"

    invoke-virtual {v0, v1}, Landroid/webkit/WebView;->loadUrl(Ljava/lang/String;)V

    .line 131
    :cond_1
    return-void
.end method

.method private synthetic lambda$show$1()V
    .locals 1

    .line 145
    const-string v0, "window.LoadingBridge&&LoadingBridge.connecting()"

    invoke-direct {p0, v0}, Lcom/gta/game/ui/LoadingScreen;->evaluate(Ljava/lang/String;)V

    return-void
.end method

.method private static quote(Ljava/lang/String;)Ljava/lang/String;
    .locals 5
    .param p0, "value"    # Ljava/lang/String;

    .line 112
    if-nez p0, :cond_0

    const-string v0, "\"\""

    return-object v0

    .line 113
    :cond_0
    new-instance v0, Ljava/lang/StringBuilder;

    invoke-direct {v0}, Ljava/lang/StringBuilder;-><init>()V

    const-string v1, "\""

    invoke-virtual {v0, v1}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    move-result-object v0

    const-string v2, "\\"

    const-string v3, "\\\\"

    invoke-virtual {p0, v2, v3}, Ljava/lang/String;->replace(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Ljava/lang/String;

    move-result-object v2

    const-string v3, "\\\""

    invoke-virtual {v2, v1, v3}, Ljava/lang/String;->replace(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Ljava/lang/String;

    move-result-object v2

    const-string v3, "\n"

    const-string v4, " "

    invoke-virtual {v2, v3, v4}, Ljava/lang/String;->replace(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Ljava/lang/String;

    move-result-object v2

    invoke-virtual {v0, v2}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    move-result-object v0

    invoke-virtual {v0, v1}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    move-result-object v0

    invoke-virtual {v0}, Ljava/lang/StringBuilder;->toString()Ljava/lang/String;

    move-result-object v0

    return-object v0
.end method

.method private runOnUiThread(Ljava/lang/Runnable;)V
    .locals 1
    .param p1, "action"    # Ljava/lang/Runnable;

    .line 101
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->activity:Landroid/app/Activity;

    if-eqz v0, :cond_1

    if-nez p1, :cond_0

    goto :goto_0

    .line 102
    :cond_0
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->activity:Landroid/app/Activity;

    invoke-virtual {v0, p1}, Landroid/app/Activity;->runOnUiThread(Ljava/lang/Runnable;)V

    .line 103
    return-void

    .line 101
    :cond_1
    :goto_0
    return-void
.end method


# virtual methods
.method public destroy()V
    .locals 3

    .line 151
    invoke-virtual {p0}, Lcom/gta/game/ui/LoadingScreen;->hide()V

    .line 152
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    const/4 v1, 0x0

    if-eqz v0, :cond_0

    .line 153
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    invoke-virtual {v0}, Landroid/webkit/WebView;->stopLoading()V

    .line 154
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    const-string v2, "AndroidLoading"

    invoke-virtual {v0, v2}, Landroid/webkit/WebView;->removeJavascriptInterface(Ljava/lang/String;)V

    .line 155
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    invoke-virtual {v0}, Landroid/webkit/WebView;->destroy()V

    .line 156
    iput-object v1, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    .line 158
    :cond_0
    iput-object v1, p0, Lcom/gta/game/ui/LoadingScreen;->handler:Landroid/os/Handler;

    .line 159
    iput-object v1, p0, Lcom/gta/game/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    .line 160
    return-void
.end method

.method public hide()V
    .locals 3

    .line 117
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->handler:Landroid/os/Handler;

    const/4 v1, 0x0

    if-eqz v0, :cond_0

    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->handler:Landroid/os/Handler;

    invoke-virtual {v0, v1}, Landroid/os/Handler;->removeCallbacksAndMessages(Ljava/lang/Object;)V

    .line 119
    :cond_0
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    if-eqz v0, :cond_3

    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    invoke-virtual {v0}, Landroidx/constraintlayout/widget/ConstraintLayout;->getVisibility()I

    move-result v0

    if-nez v0, :cond_3

    iget-boolean v0, p0, Lcom/gta/game/ui/LoadingScreen;->hiding:Z

    if-eqz v0, :cond_1

    goto :goto_0

    .line 120
    :cond_1
    const/4 v0, 0x1

    iput-boolean v0, p0, Lcom/gta/game/ui/LoadingScreen;->hiding:Z

    .line 121
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    if-eqz v0, :cond_2

    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    const-string v2, "window.LoadingBridge&&LoadingBridge.stopMusic&&LoadingBridge.stopMusic()"

    invoke-virtual {v0, v2, v1}, Landroid/webkit/WebView;->evaluateJavascript(Ljava/lang/String;Landroid/webkit/ValueCallback;)V

    .line 123
    :cond_2
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    invoke-virtual {v0}, Landroidx/constraintlayout/widget/ConstraintLayout;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const/4 v1, 0x0

    invoke-virtual {v0, v1}, Landroid/view/ViewPropertyAnimator;->alpha(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const-wide/16 v1, 0x15e

    invoke-virtual {v0, v1, v2}, Landroid/view/ViewPropertyAnimator;->setDuration(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    new-instance v1, Lcom/gta/game/ui/LoadingScreen$$ExternalSyntheticLambda2;

    invoke-direct {v1, p0}, Lcom/gta/game/ui/LoadingScreen$$ExternalSyntheticLambda2;-><init>(Lcom/gta/game/ui/LoadingScreen;)V

    invoke-virtual {v0, v1}, Landroid/view/ViewPropertyAnimator;->withEndAction(Ljava/lang/Runnable;)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    .line 131
    invoke-virtual {v0}, Landroid/view/ViewPropertyAnimator;->start()V

    .line 132
    return-void

    .line 119
    :cond_3
    :goto_0
    return-void
.end method

.method public show()V
    .locals 4

    .line 135
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    if-eqz v0, :cond_5

    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    if-nez v0, :cond_0

    goto :goto_0

    .line 136
    :cond_0
    iget-boolean v0, p0, Lcom/gta/game/ui/LoadingScreen;->hiding:Z

    const/4 v1, 0x0

    if-eqz v0, :cond_1

    .line 137
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    invoke-virtual {v0}, Landroidx/constraintlayout/widget/ConstraintLayout;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    invoke-virtual {v0}, Landroid/view/ViewPropertyAnimator;->cancel()V

    .line 138
    iput-boolean v1, p0, Lcom/gta/game/ui/LoadingScreen;->hiding:Z

    .line 139
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    const/high16 v2, 0x3f800000    # 1.0f

    invoke-virtual {v0, v2}, Landroidx/constraintlayout/widget/ConstraintLayout;->setAlpha(F)V

    .line 141
    :cond_1
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    invoke-virtual {v0}, Landroidx/constraintlayout/widget/ConstraintLayout;->getVisibility()I

    move-result v0

    if-nez v0, :cond_2

    return-void

    .line 142
    :cond_2
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    invoke-virtual {v0, v1}, Landroidx/constraintlayout/widget/ConstraintLayout;->setVisibility(I)V

    .line 143
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    if-eqz v0, :cond_3

    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->loadingWebView:Landroid/webkit/WebView;

    const-string v1, "file:///android_asset/loading/index.html"

    invoke-virtual {v0, v1}, Landroid/webkit/WebView;->loadUrl(Ljava/lang/String;)V

    .line 144
    :cond_3
    iget-boolean v0, p0, Lcom/gta/game/ui/LoadingScreen;->completed:Z

    if-eqz v0, :cond_4

    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->handler:Landroid/os/Handler;

    if-eqz v0, :cond_4

    .line 145
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->handler:Landroid/os/Handler;

    new-instance v1, Lcom/gta/game/ui/LoadingScreen$$ExternalSyntheticLambda0;

    invoke-direct {v1, p0}, Lcom/gta/game/ui/LoadingScreen$$ExternalSyntheticLambda0;-><init>(Lcom/gta/game/ui/LoadingScreen;)V

    const-wide/16 v2, 0x258

    invoke-virtual {v0, v1, v2, v3}, Landroid/os/Handler;->postDelayed(Ljava/lang/Runnable;J)Z

    .line 146
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen;->handler:Landroid/os/Handler;

    new-instance v1, Lcom/gta/game/ui/LoadingScreen$$ExternalSyntheticLambda1;

    invoke-direct {v1, p0}, Lcom/gta/game/ui/LoadingScreen$$ExternalSyntheticLambda1;-><init>(Lcom/gta/game/ui/LoadingScreen;)V

    const-wide/16 v2, 0x7530

    invoke-virtual {v0, v1, v2, v3}, Landroid/os/Handler;->postDelayed(Ljava/lang/Runnable;J)Z

    .line 148
    :cond_4
    return-void

    .line 135
    :cond_5
    :goto_0
    return-void
.end method
