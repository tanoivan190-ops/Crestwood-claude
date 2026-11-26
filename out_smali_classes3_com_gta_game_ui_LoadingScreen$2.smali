.class Lcom/gta/game/ui/LoadingScreen$2;
.super Ljava/lang/Object;
.source "LoadingScreen.java"

# interfaces
.implements Lcom/rockstargames/oswrapper/DataInstaller$Listener;


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
.method public static synthetic $r8$lambda$0pdazXJLk1ZjUCO7waw7qoMfGUk(Lcom/gta/game/ui/LoadingScreen$2;)V
    .locals 0

    invoke-direct {p0}, Lcom/gta/game/ui/LoadingScreen$2;->lambda$onComplete$1()V

    return-void
.end method

.method public static synthetic $r8$lambda$Dz3vXfKmHaabeT5NsIvER9f78Ac(Lcom/gta/game/ui/LoadingScreen$2;ILjava/lang/String;JJ)V
    .locals 0

    invoke-direct/range {p0 .. p6}, Lcom/gta/game/ui/LoadingScreen$2;->lambda$onProgress$0(ILjava/lang/String;JJ)V

    return-void
.end method

.method public static synthetic $r8$lambda$lZ72zle3kyUn05hgZsK1qjJYWos(Lcom/gta/game/ui/LoadingScreen$2;Ljava/lang/String;)V
    .locals 0

    invoke-direct {p0, p1}, Lcom/gta/game/ui/LoadingScreen$2;->lambda$onError$2(Ljava/lang/String;)V

    return-void
.end method

.method constructor <init>(Lcom/gta/game/ui/LoadingScreen;)V
    .locals 0
    .param p1, "this$0"    # Lcom/gta/game/ui/LoadingScreen;

    .line 77
    iput-object p1, p0, Lcom/gta/game/ui/LoadingScreen$2;->this$0:Lcom/gta/game/ui/LoadingScreen;

    invoke-direct {p0}, Ljava/lang/Object;-><init>()V

    return-void
.end method

.method private synthetic lambda$onComplete$1()V
    .locals 5

    .line 83
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$2;->this$0:Lcom/gta/game/ui/LoadingScreen;

    const/4 v1, 0x1

    invoke-static {v0, v1}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$fputcompleted(Lcom/gta/game/ui/LoadingScreen;Z)V

    .line 86
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$2;->this$0:Lcom/gta/game/ui/LoadingScreen;

    const-string v1, "window.LoadingBridge&&LoadingBridge.connecting()"

    invoke-static {v0, v1}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$mevaluate(Lcom/gta/game/ui/LoadingScreen;Ljava/lang/String;)V

    .line 87
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$2;->this$0:Lcom/gta/game/ui/LoadingScreen;

    invoke-static {v0}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$fgethandler(Lcom/gta/game/ui/LoadingScreen;)Landroid/os/Handler;

    move-result-object v0

    if-eqz v0, :cond_0

    .line 88
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$2;->this$0:Lcom/gta/game/ui/LoadingScreen;

    invoke-static {v0}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$fgethandler(Lcom/gta/game/ui/LoadingScreen;)Landroid/os/Handler;

    move-result-object v0

    const/4 v1, 0x0

    invoke-virtual {v0, v1}, Landroid/os/Handler;->removeCallbacksAndMessages(Ljava/lang/Object;)V

    .line 89
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$2;->this$0:Lcom/gta/game/ui/LoadingScreen;

    invoke-static {v0}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$fgethandler(Lcom/gta/game/ui/LoadingScreen;)Landroid/os/Handler;

    move-result-object v0

    iget-object v1, p0, Lcom/gta/game/ui/LoadingScreen$2;->this$0:Lcom/gta/game/ui/LoadingScreen;

    new-instance v2, Lcom/gta/game/ui/LoadingScreen$$ExternalSyntheticLambda1;

    invoke-direct {v2, v1}, Lcom/gta/game/ui/LoadingScreen$$ExternalSyntheticLambda1;-><init>(Lcom/gta/game/ui/LoadingScreen;)V

    const-wide/16 v3, 0x7530

    invoke-virtual {v0, v2, v3, v4}, Landroid/os/Handler;->postDelayed(Ljava/lang/Runnable;J)Z

    .line 91
    :cond_0
    return-void
.end method

.method private synthetic lambda$onError$2(Ljava/lang/String;)V
    .locals 3
    .param p1, "message"    # Ljava/lang/String;

    .line 94
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$2;->this$0:Lcom/gta/game/ui/LoadingScreen;

    new-instance v1, Ljava/lang/StringBuilder;

    invoke-direct {v1}, Ljava/lang/StringBuilder;-><init>()V

    const-string v2, "window.LoadingBridge&&LoadingBridge.error("

    invoke-virtual {v1, v2}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    move-result-object v1

    invoke-static {p1}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$smquote(Ljava/lang/String;)Ljava/lang/String;

    move-result-object v2

    invoke-virtual {v1, v2}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    move-result-object v1

    const-string v2, ")"

    invoke-virtual {v1, v2}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    move-result-object v1

    invoke-virtual {v1}, Ljava/lang/StringBuilder;->toString()Ljava/lang/String;

    move-result-object v1

    invoke-static {v0, v1}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$mevaluate(Lcom/gta/game/ui/LoadingScreen;Ljava/lang/String;)V

    return-void
.end method

.method private synthetic lambda$onProgress$0(ILjava/lang/String;JJ)V
    .locals 4
    .param p1, "percent"    # I
    .param p2, "phase"    # Ljava/lang/String;
    .param p3, "done"    # J
    .param p5, "total"    # J

    .line 79
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$2;->this$0:Lcom/gta/game/ui/LoadingScreen;

    new-instance v1, Ljava/lang/StringBuilder;

    invoke-direct {v1}, Ljava/lang/StringBuilder;-><init>()V

    const-string v2, "window.LoadingBridge&&LoadingBridge.setProgress("

    invoke-virtual {v1, v2}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    move-result-object v1

    invoke-virtual {v1, p1}, Ljava/lang/StringBuilder;->append(I)Ljava/lang/StringBuilder;

    move-result-object v1

    const-string v2, ","

    invoke-virtual {v1, v2}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    move-result-object v1

    invoke-static {p2}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$smquote(Ljava/lang/String;)Ljava/lang/String;

    move-result-object v3

    invoke-virtual {v1, v3}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    move-result-object v1

    invoke-virtual {v1, v2}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    move-result-object v1

    invoke-virtual {v1, p3, p4}, Ljava/lang/StringBuilder;->append(J)Ljava/lang/StringBuilder;

    move-result-object v1

    invoke-virtual {v1, v2}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    move-result-object v1

    invoke-virtual {v1, p5, p6}, Ljava/lang/StringBuilder;->append(J)Ljava/lang/StringBuilder;

    move-result-object v1

    const-string v2, ")"

    invoke-virtual {v1, v2}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    move-result-object v1

    invoke-virtual {v1}, Ljava/lang/StringBuilder;->toString()Ljava/lang/String;

    move-result-object v1

    invoke-static {v0, v1}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$mevaluate(Lcom/gta/game/ui/LoadingScreen;Ljava/lang/String;)V

    return-void
.end method


# virtual methods
.method public onComplete(Ljava/io/File;)V
    .locals 2
    .param p1, "root"    # Ljava/io/File;

    .line 82
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$2;->this$0:Lcom/gta/game/ui/LoadingScreen;

    new-instance v1, Lcom/gta/game/ui/LoadingScreen$2$$ExternalSyntheticLambda0;

    invoke-direct {v1, p0}, Lcom/gta/game/ui/LoadingScreen$2$$ExternalSyntheticLambda0;-><init>(Lcom/gta/game/ui/LoadingScreen$2;)V

    invoke-static {v0, v1}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$mrunOnUiThread(Lcom/gta/game/ui/LoadingScreen;Ljava/lang/Runnable;)V

    .line 92
    return-void
.end method

.method public onError(Ljava/lang/String;Ljava/lang/Throwable;)V
    .locals 2
    .param p1, "message"    # Ljava/lang/String;
    .param p2, "error"    # Ljava/lang/Throwable;

    .line 94
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$2;->this$0:Lcom/gta/game/ui/LoadingScreen;

    new-instance v1, Lcom/gta/game/ui/LoadingScreen$2$$ExternalSyntheticLambda2;

    invoke-direct {v1, p0, p1}, Lcom/gta/game/ui/LoadingScreen$2$$ExternalSyntheticLambda2;-><init>(Lcom/gta/game/ui/LoadingScreen$2;Ljava/lang/String;)V

    invoke-static {v0, v1}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$mrunOnUiThread(Lcom/gta/game/ui/LoadingScreen;Ljava/lang/Runnable;)V

    .line 95
    return-void
.end method

.method public onProgress(ILjava/lang/String;JJ)V
    .locals 9
    .param p1, "percent"    # I
    .param p2, "phase"    # Ljava/lang/String;
    .param p3, "done"    # J
    .param p5, "total"    # J

    .line 79
    iget-object v0, p0, Lcom/gta/game/ui/LoadingScreen$2;->this$0:Lcom/gta/game/ui/LoadingScreen;

    new-instance v1, Lcom/gta/game/ui/LoadingScreen$2$$ExternalSyntheticLambda1;

    move-object v2, p0

    move v3, p1

    move-object v4, p2

    move-wide v5, p3

    move-wide v7, p5

    .end local p1    # "percent":I
    .end local p2    # "phase":Ljava/lang/String;
    .end local p3    # "done":J
    .end local p5    # "total":J
    .local v3, "percent":I
    .local v4, "phase":Ljava/lang/String;
    .local v5, "done":J
    .local v7, "total":J
    invoke-direct/range {v1 .. v8}, Lcom/gta/game/ui/LoadingScreen$2$$ExternalSyntheticLambda1;-><init>(Lcom/gta/game/ui/LoadingScreen$2;ILjava/lang/String;JJ)V

    invoke-static {v0, v1}, Lcom/gta/game/ui/LoadingScreen;->-$$Nest$mrunOnUiThread(Lcom/gta/game/ui/LoadingScreen;Ljava/lang/Runnable;)V

    .line 80
    return-void
.end method
