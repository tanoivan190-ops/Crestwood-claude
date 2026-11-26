.class public Lcom/gta/game32/ui/LoadingScreen;
.super Ljava/lang/Object;
.source "LoadingScreen.java"


# instance fields
.field private activity:Landroid/app/Activity;

.field private mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;


# direct methods
.method public constructor <init>(Landroid/app/Activity;)V
    .locals 3
    .param p1, "activity"    # Landroid/app/Activity;

    .line 16
    invoke-direct {p0}, Ljava/lang/Object;-><init>()V

    .line 17
    iput-object p1, p0, Lcom/gta/game32/ui/LoadingScreen;->activity:Landroid/app/Activity;

    .line 19
    invoke-virtual {p1}, Landroid/app/Activity;->getLayoutInflater()Landroid/view/LayoutInflater;

    move-result-object v0

    sget v1, Lcom/gta/game/R$layout;->loadingscreen:I

    const/4 v2, 0x0

    invoke-virtual {v0, v1, v2}, Landroid/view/LayoutInflater;->inflate(ILandroid/view/ViewGroup;)Landroid/view/View;

    move-result-object v0

    check-cast v0, Landroidx/constraintlayout/widget/ConstraintLayout;

    iput-object v0, p0, Lcom/gta/game32/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    .line 20
    iget-object v0, p0, Lcom/gta/game32/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    new-instance v1, Landroidx/constraintlayout/widget/ConstraintLayout$LayoutParams;

    const/4 v2, -0x1

    invoke-direct {v1, v2, v2}, Landroidx/constraintlayout/widget/ConstraintLayout$LayoutParams;-><init>(II)V

    invoke-virtual {p1, v0, v1}, Landroid/app/Activity;->addContentView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 21
    return-void
.end method


# virtual methods
.method public hide()V
    .locals 2

    .line 25
    iget-object v0, p0, Lcom/gta/game32/ui/LoadingScreen;->mainLayout:Landroidx/constraintlayout/widget/ConstraintLayout;

    const/4 v1, 0x4

    invoke-virtual {v0, v1}, Landroidx/constraintlayout/widget/ConstraintLayout;->setVisibility(I)V

    .line 26
    return-void
.end method
