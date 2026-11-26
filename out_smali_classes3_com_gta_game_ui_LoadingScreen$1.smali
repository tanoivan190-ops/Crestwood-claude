.class Lcom/gta/game/ui/LoadingScreen$1;
.super Landroid/webkit/WebViewClient;
.source "LoadingScreen.java"


# annotations
.annotation system Ldalvik/annotation/EnclosingMethod;
    value = Lcom/gta/game/ui/LoadingScreen;->initializeViews()V
.end annotation

.annotation system Ldalvik/annotation/InnerClass;
    accessFlags = 0x0
    name = null
.end annotation


# instance fields
.field final synthetic this$0:Lcom/gta/game/ui/LoadingScreen;


# direct methods
.method public static synthetic $r8$lambda$j5h-NXsgnb5RS0AreXye1oopUNI(Lcom/gta/game/ui/LoadingScreen$1;Landroid/webkit/WebView;)V
    .locals 0

    invoke-direct {p0, p1}, Lcom/gta/game/ui/LoadingScreen$1;->lambda$onRenderProcessGone$0(Landroid/webkit/WebView;)V

    return-void
.end method

.method constructor <init>(Lcom/gta/game/ui/LoadingScreen;)V
    .locals 0
    .param p1, "this$0"    # Lcom/gta/game/ui/LoadingScreen;

    .line 43
    iput-object p1, p0, Lcom/gta/game/ui/LoadingScreen$1;->this$0:Lcom/gta/game/ui/LoadingScreen;

    invoke-direct {p0}, Landroid/webkit/WebViewClient;-><init>()V

    return-void
.end method

.method private synthetic lambda$onRenderProcessGone$0(Landroid/webkit/WebView;)V
    .locals 2
    .param p1, "view"    # Landroid/webkit/WebView;

    .line 49
    :try_start_0
    invoke-virtual {p1}, Landroid/webkit/WebView;->getParent()Landroid/view/ViewParent;

    move-result-object v0

    check-cast v0, Landroid/view/ViewGroup;

    .line 50
    .local v0, "parent":Landroid/view/ViewGroup;
    if-eqz v0, :cond_0

    invoke-virtual {v0, p1}, Landroid/view/ViewGroup;->removeView(Landroid/view/View;)V

    .line 51
    :cond_0
    invoke-virtual {p1}, Landroid/webkit/WebView;->destroy()V
    :try_end_0
    .catchall {:try_start_0 .. :try_end_0} :catchall_0

    .end local v0    # "parent":Landroid/view/ViewGroup;
    goto :goto_0

    .line 52
    :catchall_0
    move-exception v0

    :goto_0
    nop

    .line 53
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$1;->this$0:Lcom/gta/game/ui/LoadingScreen;

    invoke-static {v0}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$fgetloadingWebView(Lcom/gta/game/ui/LoadingScreen;)Landroid/webkit/WebView;

    move-result-object v0

    const/4 v1, 0x0

    if-ne v0, p1, :cond_1

    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$1;->this$0:Lcom/gta/game/ui/LoadingScreen;

    invoke-static {v0, v1}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$fputloadingWebView(Lcom/gta/game/ui/LoadingScreen;Landroid/webkit/WebView;)V

    .line 54
    :cond_1
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$1;->this$0:Lcom/gta/game/ui/LoadingScreen;

    invoke-static {v0}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$fgethandler(Lcom/gta/game/ui/LoadingScreen;)Landroid/os/Handler;

    move-result-object v0

    if-eqz v0, :cond_2

    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$1;->this$0:Lcom/gta/game/ui/LoadingScreen;

    invoke-static {v0}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$fgethandler(Lcom/gta/game/ui/LoadingScreen;)Landroid/os/Handler;

    move-result-object v0

    invoke-virtual {v0, v1}, Landroid/os/Handler;->removeCallbacksAndMessages(Ljava/lang/Object;)V

    .line 55
    :cond_2
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$1;->this$0:Lcom/gta/game/ui/LoadingScreen;

    invoke-static {v0}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$fgetmainLayout(Lcom/gta/game/ui/LoadingScreen;)Landroidx/constraintlayout/widget/ConstraintLayout;

    move-result-object v0

    if-eqz v0, :cond_3

    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$1;->this$0:Lcom/gta/game/ui/LoadingScreen;

    invoke-static {v0}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$fgetmainLayout(Lcom/gta/game/ui/LoadingScreen;)Landroidx/constraintlayout/widget/ConstraintLayout;

    move-result-object v0

    const/16 v1, 0x8

    invoke-virtual {v0, v1}, Landroidx/constraintlayout/widget/ConstraintLayout;->setVisibility(I)V

    .line 56
    :cond_3
    return-void
.end method


# virtual methods
.method public onPageFinished(Landroid/webkit/WebView;Ljava/lang/String;)V
    .locals 2
    .param p1, "view"    # Landroid/webkit/WebView;
    .param p2, "url"    # Ljava/lang/String;

    .line 61
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$1;->this$0:Lcom/gta/game/ui/LoadingScreen;

    invoke-static {v0}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$fgetcompleted(Lcom/gta/game/ui/LoadingScreen;)Z

    move-result v0

    if-eqz v0, :cond_0

    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$1;->this$0:Lcom/gta/game/ui/LoadingScreen;

    const-string v1, "window.LoadingBridge&&LoadingBridge.connecting()"

    invoke-static {v0, v1}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$mevaluate(Lcom/gta/game/ui/LoadingScreen;Ljava/lang/String;)V

    .line 62
    :cond_0
    return-void
.end method

.method public onRenderProcessGone(Landroid/webkit/WebView;Landroid/webkit/RenderProcessGoneDetail;)Z
    .locals 2
    .param p1, "view"    # Landroid/webkit/WebView;
    .param p2, "detail"    # Landroid/webkit/RenderProcessGoneDetail;

    .line 46
    const-string v0, "LoadingScreen"

    const-string v1, "[webview] moteur de rendu perdu : ecran de chargement retire"

    invoke-static {v0, v1}, Landroid/util/Log;->e(Ljava/lang/String;Ljava/lang/String;)I

    .line 47
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$1;->this$0:Lcom/gta/game/ui/LoadingScreen;

    new-instance v1, Lcom/gta/game/ui/LoadingScreen$1$$ExternalSyntheticLambda0;

    invoke-direct {v1, p0, p1}, Lcom/gta/game/ui/LoadingScreen$1$$ExternalSyntheticLambda0;-><init>(Lcom/gta/game/ui/LoadingScreen$1;Landroid/webkit/WebView;)V

    invoke-static {v0, v1}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$mrunOnUiThread(Lcom/gta/game/ui/LoadingScreen;Ljava/lang/Runnable;)V

    .line 57
    const/4 v0, 0x1

    return v0
.end method
