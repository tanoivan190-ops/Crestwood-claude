.class final Lcom/gta/game/ui/LoadingScreen$LoadingBridge;
.super Ljava/lang/Object;
.source "LoadingScreen.java"


# annotations
.annotation system Ldalvik/annotation/EnclosingClass;
    value = Lcom/gta/game/ui/LoadingScreen;
.end annotation

.annotation system Ldalvik/annotation/InnerClass;
    accessFlags = 0x12
    name = "LoadingBridge"
.end annotation


# instance fields
.field final synthetic this$0:Lcom/gta/game/ui/LoadingScreen;


# direct methods
.method private constructor <init>(Lcom/gta/game/ui/LoadingScreen;)V
    .locals 0

    .line 162
    iput-object p1, p0, Lcom/gta/game/ui/LoadingScreen$LoadingBridge;->this$0:Lcom/gta/game/ui/LoadingScreen;

    invoke-direct {p0}, Ljava/lang/Object;-><init>()V

    return-void
.end method

.method synthetic constructor <init>(Lcom/gta/game/ui/LoadingScreen;Lcom/gta/game/ui/LoadingScreen-IA;)V
    .locals 0

    invoke-direct {p0, p1}, Lcom/gta/game/ui/LoadingScreen$LoadingBridge;-><init>(Lcom/gta/game/ui/LoadingScreen;)V

    return-void
.end method


# virtual methods
.method public retry()V
    .locals 1
    .annotation runtime Landroid/webkit/JavascriptInterface;
    .end annotation

    .line 164
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$LoadingBridge;->this$0:Lcom/gta/game/ui/LoadingScreen;

    invoke-static {v0}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$fgetactivity(Lcom/gta/game/ui/LoadingScreen;)Landroid/app/Activity;

    move-result-object v0

    invoke-static {v0}, Lcom/rockstargames/oswrapper/DataInstaller;->repair(Landroid/content/Context;)V

    return-void
.end method

.method public skip()V
    .locals 3
    .annotation runtime Landroid/webkit/JavascriptInterface;
    .end annotation

    .line 166
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$LoadingBridge;->this$0:Lcom/gta/game/ui/LoadingScreen;

    iget-object v1, p0, Lcom/gta/game/ui/LoadingScreen$LoadingBridge;->this$0:Lcom/gta/game/ui/LoadingScreen;

    new-instance v2, Lcom/gta/game/ui/LoadingScreen$$ExternalSyntheticLambda1;

    invoke-direct {v2, v1}, Lcom/gta/game/ui/LoadingScreen$$ExternalSyntheticLambda1;-><init>(Lcom/gta/game/ui/LoadingScreen;)V

    invoke-static {v0, v2}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$mrunOnUiThread(Lcom/gta/game/ui/LoadingScreen;Ljava/lang/Runnable;)V

    return-void
.end method

.method public start()V
    .locals 1
    .annotation runtime Landroid/webkit/JavascriptInterface;
    .end annotation

    .line 163
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$LoadingBridge;->this$0:Lcom/gta/game/ui/LoadingScreen;

    invoke-static {v0}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$fgetactivity(Lcom/gta/game/ui/LoadingScreen;)Landroid/app/Activity;

    move-result-object v0

    invoke-static {v0}, Lcom/rockstargames/oswrapper/DataInstaller;->prepare(Landroid/content/Context;)V

    return-void
.end method
