.class public final Lcom/gta/launcher/ui/IntroOverlay;
.super Ljava/lang/Object;
.source "IntroOverlay.java"


# static fields
.field private static final CAPTIONS:[[Ljava/lang/String;

.field private static final CROSSFADE_MS:J = 0x514L

.field private static final FADE_OUT_MS:J = 0x578L

.field private static final FINALE_MS:J = 0x15e0L

.field private static final MUSIC_START_MS:I = 0x0

.field private static final MUSIC_VOLUME:F = 0.9f

.field private static final SKIP_ALLOWED_AT_MS:J = 0x2ee0L

.field private static final SLIDES:[I

.field private static final SLIDE_MS:J = 0x11f8L

.field private static final SLIDE_PASSES:I = 0x4

.field private static final TAG:Ljava/lang/String; = "IntroOverlay"

.field private static final TOTAL_MS:J = 0x5dc0L

.field private static shownThisProcess:Z


# instance fields
.field private final activity:Landroid/app/Activity;

.field private back:Landroid/widget/ImageView;

.field private captionBox:Landroid/widget/LinearLayout;

.field private captionKicker:Landroid/widget/TextView;

.field private captionLine:Landroid/widget/TextView;

.field private copyright:Landroid/widget/TextView;

.field private fadeAnimator:Landroid/animation/ObjectAnimator;

.field private fadingOut:Z

.field private finished:Z

.field private front:Landroid/widget/ImageView;

.field private final handler:Landroid/os/Handler;

.field private final onFinished:Ljava/lang/Runnable;

.field private player:Landroid/media/MediaPlayer;

.field private presents:Landroid/widget/TextView;

.field private progressFill:Landroid/view/View;

.field private root:Landroid/widget/FrameLayout;

.field private skipAllowed:Z

.field private skipHint:Landroid/widget/TextView;

.field private startedAt:J

.field private studioBox:Landroid/widget/LinearLayout;

.field private topMark:Landroid/view/View;


# direct methods
.method public static synthetic $r8$lambda$-ZDIr9dSQ8vYfXY6e7srLYjDr2Q(Lcom/gta/launcher/ui/IntroOverlay;Landroid/view/View;)V
    .locals 0

    invoke-direct {p0, p1}, Lcom/gta/launcher/ui/IntroOverlay;->lambda$build$0(Landroid/view/View;)V

    return-void
.end method

.method public static synthetic $r8$lambda$16JyNUPDt2Q3ld8tpciULJBdtws(Lcom/gta/launcher/ui/IntroOverlay;)V
    .locals 0

    invoke-direct {p0}, Lcom/gta/launcher/ui/IntroOverlay;->showFinale()V

    return-void
.end method

.method public static synthetic $r8$lambda$5r3m5fZocOBXxlq8I3X3wH-ZNqQ(Lcom/gta/launcher/ui/IntroOverlay;[Ljava/lang/String;)V
    .locals 0

    invoke-direct {p0, p1}, Lcom/gta/launcher/ui/IntroOverlay;->lambda$showCaption$5([Ljava/lang/String;)V

    return-void
.end method

.method public static synthetic $r8$lambda$AnCrBFYd4tcewlVNV9Ks3G2CoaE(Lcom/gta/launcher/ui/IntroOverlay;)V
    .locals 0

    invoke-direct {p0}, Lcom/gta/launcher/ui/IntroOverlay;->lambda$showFinale$7()V

    return-void
.end method

.method public static synthetic $r8$lambda$QGgL8d-vqu3Qv5X3nrKQw3Lpe54(Lcom/gta/launcher/ui/IntroOverlay;)V
    .locals 0

    invoke-direct {p0}, Lcom/gta/launcher/ui/IntroOverlay;->lambda$showCaption$4()V

    return-void
.end method

.method public static synthetic $r8$lambda$VfwA3-sLRokiWGzSeQN9YGrEfRY(Lcom/gta/launcher/ui/IntroOverlay;)V
    .locals 0

    invoke-direct {p0}, Lcom/gta/launcher/ui/IntroOverlay;->lambda$showFinale$6()V

    return-void
.end method

.method public static synthetic $r8$lambda$YvndK3n55XwE1a9ud7co2i-QD0w(Lcom/gta/launcher/ui/IntroOverlay;)V
    .locals 0

    invoke-direct {p0}, Lcom/gta/launcher/ui/IntroOverlay;->lambda$start$3()V

    return-void
.end method

.method public static synthetic $r8$lambda$_p2ZZBNneCwFfCol1Nbpx-oVSKw(Lcom/gta/launcher/ui/IntroOverlay;)V
    .locals 0

    invoke-direct {p0}, Lcom/gta/launcher/ui/IntroOverlay;->finish()V

    return-void
.end method

.method public static synthetic $r8$lambda$rJkDOA59A4NeUDp7gxDSDHh5fdA(Lcom/gta/launcher/ui/IntroOverlay;I)V
    .locals 0

    invoke-direct {p0, p1}, Lcom/gta/launcher/ui/IntroOverlay;->lambda$start$1(I)V

    return-void
.end method

.method public static synthetic $r8$lambda$vgmf3dNeJAgvVY6xgLg9mz_rIm4(Lcom/gta/launcher/ui/IntroOverlay;)V
    .locals 0

    invoke-direct {p0}, Lcom/gta/launcher/ui/IntroOverlay;->lambda$start$2()V

    return-void
.end method

.method static bridge synthetic -$$Nest$fgetfinished(Lcom/gta/launcher/ui/IntroOverlay;)Z
    .locals 0

    iget-boolean p0, p0, Lcom/gta/launcher/ui/IntroOverlay;->finished:Z

    return p0
.end method

.method static bridge synthetic -$$Nest$fgethandler(Lcom/gta/launcher/ui/IntroOverlay;)Landroid/os/Handler;
    .locals 0

    iget-object p0, p0, Lcom/gta/launcher/ui/IntroOverlay;->handler:Landroid/os/Handler;

    return-object p0
.end method

.method static bridge synthetic -$$Nest$fgetplayer(Lcom/gta/launcher/ui/IntroOverlay;)Landroid/media/MediaPlayer;
    .locals 0

    iget-object p0, p0, Lcom/gta/launcher/ui/IntroOverlay;->player:Landroid/media/MediaPlayer;

    return-object p0
.end method

.method static constructor <clinit>()V
    .locals 6

    .line 38
    sget v0, Lcom/gta/game/R$drawable;->intro_slide_1:I

    sget v1, Lcom/gta/game/R$drawable;->intro_slide_2:I

    filled-new-array {v0, v1}, [I

    move-result-object v0

    sput-object v0, Lcom/gta/launcher/ui/IntroOverlay;->SLIDES:[I

    .line 40
    const/4 v0, 0x4

    new-array v0, v0, [[Ljava/lang/String;

    const/4 v1, 0x2

    new-array v2, v1, [Ljava/lang/String;

    const-string v3, "LOS SANTOS"

    const/4 v4, 0x0

    aput-object v3, v2, v4

    const-string v3, "Bienvenue \u00e0 Crestwood"

    const/4 v5, 0x1

    aput-object v3, v2, v5

    aput-object v2, v0, v4

    new-array v2, v1, [Ljava/lang/String;

    const-string v3, "LIBERT\u00c9"

    aput-object v3, v2, v4

    const-string v3, "Roule. Construis. R\u00e8gne."

    aput-object v3, v2, v5

    aput-object v2, v0, v5

    new-array v2, v1, [Ljava/lang/String;

    const-string v3, "LA RUE"

    aput-object v3, v2, v4

    const-string v3, "Ton histoire commence ici"

    aput-object v3, v2, v5

    aput-object v2, v0, v1

    new-array v1, v1, [Ljava/lang/String;

    const-string v2, "CRESTWOOD RP"

    aput-object v2, v1, v4

    const-string v2, "Le roleplay comme jamais"

    aput-object v2, v1, v5

    const/4 v2, 0x3

    aput-object v1, v0, v2

    sput-object v0, Lcom/gta/launcher/ui/IntroOverlay;->CAPTIONS:[[Ljava/lang/String;

    return-void
.end method

.method private constructor <init>(Landroid/app/Activity;Ljava/lang/Runnable;)V
    .locals 2
    .param p1, "activity"    # Landroid/app/Activity;
    .param p2, "onFinished"    # Ljava/lang/Runnable;

    .line 78
    invoke-direct {p0}, Ljava/lang/Object;-><init>()V

    .line 60
    new-instance v0, Landroid/os/Handler;

    invoke-static {}, Landroid/os/Looper;->getMainLooper()Landroid/os/Looper;

    move-result-object v1

    invoke-direct {v0, v1}, Landroid/os/Handler;-><init>(Landroid/os/Looper;)V

    iput-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->handler:Landroid/os/Handler;

    .line 79
    iput-object p1, p0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    .line 80
    iput-object p2, p0, Lcom/gta/launcher/ui/IntroOverlay;->onFinished:Ljava/lang/Runnable;

    .line 81
    return-void
.end method

.method private build()V
    .locals 30

    .line 134
    move-object/from16 v0, p0

    new-instance v1, Landroid/widget/FrameLayout;

    iget-object v2, v0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    invoke-direct {v1, v2}, Landroid/widget/FrameLayout;-><init>(Landroid/content/Context;)V

    iput-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    .line 135
    iget-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    const/high16 v2, -0x1000000

    invoke-virtual {v1, v2}, Landroid/widget/FrameLayout;->setBackgroundColor(I)V

    .line 136
    iget-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    const/4 v6, 0x1

    invoke-virtual {v1, v6}, Landroid/widget/FrameLayout;->setClickable(Z)V

    .line 137
    iget-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    invoke-virtual {v1, v6}, Landroid/widget/FrameLayout;->setFocusable(Z)V

    .line 138
    iget-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    new-instance v3, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda5;

    invoke-direct {v3, v0}, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda5;-><init>(Lcom/gta/launcher/ui/IntroOverlay;)V

    invoke-virtual {v1, v3}, Landroid/widget/FrameLayout;->setOnClickListener(Landroid/view/View$OnClickListener;)V

    .line 140
    invoke-direct {v0}, Lcom/gta/launcher/ui/IntroOverlay;->newPhoto()Landroid/widget/ImageView;

    move-result-object v1

    iput-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->back:Landroid/widget/ImageView;

    .line 141
    invoke-direct {v0}, Lcom/gta/launcher/ui/IntroOverlay;->newPhoto()Landroid/widget/ImageView;

    move-result-object v1

    iput-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->front:Landroid/widget/ImageView;

    .line 144
    sget-object v1, Landroid/graphics/drawable/GradientDrawable$Orientation;->BOTTOM_TOP:Landroid/graphics/drawable/GradientDrawable$Orientation;

    .line 145
    const/16 v3, 0xeb

    const/4 v7, 0x0

    invoke-static {v3, v7, v7, v7}, Landroid/graphics/Color;->argb(IIII)I

    move-result v3

    const/16 v4, 0x78

    invoke-static {v4, v7, v7, v7}, Landroid/graphics/Color;->argb(IIII)I

    move-result v4

    filled-new-array {v3, v4, v7}, [I

    move-result-object v3

    .line 144
    const/16 v8, 0x50

    const/16 v9, 0x96

    invoke-direct {v0, v8, v9, v1, v3}, Lcom/gta/launcher/ui/IntroOverlay;->gradient(IILandroid/graphics/drawable/GradientDrawable$Orientation;[I)Landroid/view/View;

    .line 146
    sget-object v1, Landroid/graphics/drawable/GradientDrawable$Orientation;->TOP_BOTTOM:Landroid/graphics/drawable/GradientDrawable$Orientation;

    .line 147
    const/16 v3, 0xc8

    invoke-static {v3, v7, v7, v7}, Landroid/graphics/Color;->argb(IIII)I

    move-result v4

    filled-new-array {v4, v7}, [I

    move-result-object v4

    .line 146
    const/16 v5, 0x30

    const/16 v10, 0x5a

    invoke-direct {v0, v5, v10, v1, v4}, Lcom/gta/launcher/ui/IntroOverlay;->gradient(IILandroid/graphics/drawable/GradientDrawable$Orientation;[I)Landroid/view/View;

    .line 148
    sget-object v1, Landroid/graphics/drawable/GradientDrawable$Orientation;->LEFT_RIGHT:Landroid/graphics/drawable/GradientDrawable$Orientation;

    .line 149
    invoke-static {v9, v7, v7, v7}, Landroid/graphics/Color;->argb(IIII)I

    move-result v4

    invoke-static {v9, v7, v7, v7}, Landroid/graphics/Color;->argb(IIII)I

    move-result v10

    filled-new-array {v4, v7, v7, v10}, [I

    move-result-object v4

    .line 148
    const/16 v10, 0x11

    const/4 v11, -0x1

    invoke-direct {v0, v10, v11, v1, v4}, Lcom/gta/launcher/ui/IntroOverlay;->gradient(IILandroid/graphics/drawable/GradientDrawable$Orientation;[I)Landroid/view/View;

    move-result-object v10

    .line 150
    .local v10, "sideVignette":Landroid/view/View;
    const/4 v12, 0x2

    invoke-virtual {v10, v12}, Landroid/view/View;->setImportantForAccessibility(I)V

    .line 153
    new-instance v1, Landroid/view/View;

    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    invoke-direct {v1, v4}, Landroid/view/View;-><init>(Landroid/content/Context;)V

    move-object v13, v1

    .line 154
    .local v13, "barTop":Landroid/view/View;
    invoke-virtual {v13, v2}, Landroid/view/View;->setBackgroundColor(I)V

    .line 155
    iget-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    new-instance v4, Landroid/widget/FrameLayout$LayoutParams;

    const/high16 v14, 0x41b00000    # 22.0f

    invoke-direct {v0, v14}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v15

    float-to-int v15, v15

    invoke-direct {v4, v11, v15, v5}, Landroid/widget/FrameLayout$LayoutParams;-><init>(III)V

    invoke-virtual {v1, v13, v4}, Landroid/widget/FrameLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 156
    new-instance v1, Landroid/view/View;

    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    invoke-direct {v1, v4}, Landroid/view/View;-><init>(Landroid/content/Context;)V

    move-object v15, v1

    .line 157
    .local v15, "barBottom":Landroid/view/View;
    invoke-virtual {v15, v2}, Landroid/view/View;->setBackgroundColor(I)V

    .line 158
    iget-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    new-instance v2, Landroid/widget/FrameLayout$LayoutParams;

    invoke-direct {v0, v14}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v4

    float-to-int v4, v4

    invoke-direct {v2, v11, v4, v8}, Landroid/widget/FrameLayout$LayoutParams;-><init>(III)V

    invoke-virtual {v1, v15, v2}, Landroid/widget/FrameLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 159
    const-string v1, "barTop"

    invoke-virtual {v13, v1}, Landroid/view/View;->setTag(Ljava/lang/Object;)V

    .line 160
    const-string v1, "barBottom"

    invoke-virtual {v15, v1}, Landroid/view/View;->setTag(Ljava/lang/Object;)V

    .line 163
    new-instance v1, Landroid/widget/LinearLayout;

    iget-object v2, v0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    invoke-direct {v1, v2}, Landroid/widget/LinearLayout;-><init>(Landroid/content/Context;)V

    iput-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->captionBox:Landroid/widget/LinearLayout;

    .line 164
    iget-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->captionBox:Landroid/widget/LinearLayout;

    invoke-virtual {v1, v6}, Landroid/widget/LinearLayout;->setOrientation(I)V

    .line 165
    iget-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->captionBox:Landroid/widget/LinearLayout;

    const/4 v2, 0x0

    invoke-virtual {v1, v2}, Landroid/widget/LinearLayout;->setAlpha(F)V

    .line 166
    new-instance v1, Landroid/view/View;

    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    invoke-direct {v1, v4}, Landroid/view/View;-><init>(Landroid/content/Context;)V

    .line 167
    .local v1, "accent":Landroid/view/View;
    invoke-virtual {v1, v11}, Landroid/view/View;->setBackgroundColor(I)V

    .line 168
    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->captionBox:Landroid/widget/LinearLayout;

    new-instance v5, Landroid/widget/LinearLayout$LayoutParams;

    const/high16 v2, 0x41d00000    # 26.0f

    invoke-direct {v0, v2}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v2

    float-to-int v2, v2

    const/high16 v14, 0x40000000    # 2.0f

    invoke-direct {v0, v14}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v14

    float-to-int v14, v14

    invoke-direct {v5, v2, v14}, Landroid/widget/LinearLayout$LayoutParams;-><init>(II)V

    invoke-virtual {v4, v1, v5}, Landroid/widget/LinearLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 169
    const/16 v14, 0xff

    invoke-static {v3, v14, v14, v14}, Landroid/graphics/Color;->argb(IIII)I

    move-result v3

    const/4 v4, 0x0

    const v5, 0x3ee66666    # 0.45f

    move-object v2, v1

    .end local v1    # "accent":Landroid/view/View;
    .local v2, "accent":Landroid/view/View;
    const-string v1, ""

    move-object/from16 v16, v2

    .end local v2    # "accent":Landroid/view/View;
    .local v16, "accent":Landroid/view/View;
    const/high16 v2, 0x41200000    # 10.0f

    const/4 v8, 0x0

    invoke-direct/range {v0 .. v5}, Lcom/gta/launcher/ui/IntroOverlay;->text(Ljava/lang/String;FIZF)Landroid/widget/TextView;

    move-result-object v1

    iput-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->captionKicker:Landroid/widget/TextView;

    .line 170
    new-instance v1, Landroid/widget/LinearLayout$LayoutParams;

    const/4 v2, -0x2

    invoke-direct {v1, v2, v2}, Landroid/widget/LinearLayout$LayoutParams;-><init>(II)V

    .line 171
    .local v1, "kp":Landroid/widget/LinearLayout$LayoutParams;
    const/high16 v3, 0x41000000    # 8.0f

    invoke-direct {v0, v3}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v3

    float-to-int v3, v3

    iput v3, v1, Landroid/widget/LinearLayout$LayoutParams;->topMargin:I

    .line 172
    iget-object v3, v0, Lcom/gta/launcher/ui/IntroOverlay;->captionBox:Landroid/widget/LinearLayout;

    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->captionKicker:Landroid/widget/TextView;

    invoke-virtual {v3, v4, v1}, Landroid/widget/LinearLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 173
    const/4 v4, 0x1

    const v5, 0x3ca3d70a    # 0.02f

    move-object v3, v1

    .end local v1    # "kp":Landroid/widget/LinearLayout$LayoutParams;
    .local v3, "kp":Landroid/widget/LinearLayout$LayoutParams;
    const-string v1, ""

    move/from16 v17, v2

    const/high16 v2, 0x41a80000    # 21.0f

    move-object/from16 v18, v3

    .end local v3    # "kp":Landroid/widget/LinearLayout$LayoutParams;
    .local v18, "kp":Landroid/widget/LinearLayout$LayoutParams;
    const/4 v3, -0x1

    move/from16 v11, v17

    invoke-direct/range {v0 .. v5}, Lcom/gta/launcher/ui/IntroOverlay;->text(Ljava/lang/String;FIZF)Landroid/widget/TextView;

    move-result-object v1

    iput-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->captionLine:Landroid/widget/TextView;

    .line 174
    new-instance v1, Landroid/widget/LinearLayout$LayoutParams;

    invoke-direct {v1, v11, v11}, Landroid/widget/LinearLayout$LayoutParams;-><init>(II)V

    .line 175
    .local v1, "lp":Landroid/widget/LinearLayout$LayoutParams;
    const/high16 v2, 0x40a00000    # 5.0f

    invoke-direct {v0, v2}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v3

    float-to-int v3, v3

    iput v3, v1, Landroid/widget/LinearLayout$LayoutParams;->topMargin:I

    .line 176
    iget-object v3, v0, Lcom/gta/launcher/ui/IntroOverlay;->captionBox:Landroid/widget/LinearLayout;

    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->captionLine:Landroid/widget/TextView;

    invoke-virtual {v3, v4, v1}, Landroid/widget/LinearLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 177
    new-instance v3, Landroid/widget/FrameLayout$LayoutParams;

    const v4, 0x800053

    invoke-direct {v3, v11, v11, v4}, Landroid/widget/FrameLayout$LayoutParams;-><init>(III)V

    .line 178
    .local v3, "cp":Landroid/widget/FrameLayout$LayoutParams;
    const/high16 v4, 0x42080000    # 34.0f

    invoke-direct {v0, v4}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v4

    float-to-int v4, v4

    iput v4, v3, Landroid/widget/FrameLayout$LayoutParams;->leftMargin:I

    .line 179
    const/high16 v4, 0x42300000    # 44.0f

    invoke-direct {v0, v4}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v4

    float-to-int v4, v4

    iput v4, v3, Landroid/widget/FrameLayout$LayoutParams;->bottomMargin:I

    .line 180
    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    iget-object v5, v0, Lcom/gta/launcher/ui/IntroOverlay;->captionBox:Landroid/widget/LinearLayout;

    invoke-virtual {v4, v5, v3}, Landroid/widget/FrameLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 183
    new-instance v4, Landroid/widget/LinearLayout;

    iget-object v5, v0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    invoke-direct {v4, v5}, Landroid/widget/LinearLayout;-><init>(Landroid/content/Context;)V

    iput-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->studioBox:Landroid/widget/LinearLayout;

    .line 184
    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->studioBox:Landroid/widget/LinearLayout;

    invoke-virtual {v4, v6}, Landroid/widget/LinearLayout;->setOrientation(I)V

    .line 185
    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->studioBox:Landroid/widget/LinearLayout;

    invoke-virtual {v4, v6}, Landroid/widget/LinearLayout;->setGravity(I)V

    .line 186
    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->studioBox:Landroid/widget/LinearLayout;

    invoke-virtual {v4, v8}, Landroid/widget/LinearLayout;->setAlpha(F)V

    .line 187
    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    sget v5, Lcom/gta/game/R$string;->intro_studio_name:I

    invoke-virtual {v4, v5}, Landroid/app/Activity;->getString(I)Ljava/lang/String;

    move-result-object v4

    move-object v5, v1

    move-object v1, v4

    .end local v1    # "lp":Landroid/widget/LinearLayout$LayoutParams;
    .local v5, "lp":Landroid/widget/LinearLayout$LayoutParams;
    const/4 v4, 0x1

    move-object/from16 v19, v5

    .end local v5    # "lp":Landroid/widget/LinearLayout$LayoutParams;
    .local v19, "lp":Landroid/widget/LinearLayout$LayoutParams;
    const v5, 0x3ed70a3d    # 0.42f

    move/from16 v20, v2

    const/high16 v2, 0x41500000    # 13.0f

    move-object/from16 v21, v3

    .end local v3    # "cp":Landroid/widget/FrameLayout$LayoutParams;
    .local v21, "cp":Landroid/widget/FrameLayout$LayoutParams;
    const/4 v3, -0x1

    move/from16 v12, v20

    invoke-direct/range {v0 .. v5}, Lcom/gta/launcher/ui/IntroOverlay;->text(Ljava/lang/String;FIZF)Landroid/widget/TextView;

    move-result-object v1

    .line 188
    .local v1, "studio":Landroid/widget/TextView;
    iget-object v2, v0, Lcom/gta/launcher/ui/IntroOverlay;->studioBox:Landroid/widget/LinearLayout;

    new-instance v3, Landroid/widget/LinearLayout$LayoutParams;

    invoke-direct {v3, v11, v11}, Landroid/widget/LinearLayout$LayoutParams;-><init>(II)V

    invoke-virtual {v2, v1, v3}, Landroid/widget/LinearLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 189
    iget-object v2, v0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    sget v3, Lcom/gta/game/R$string;->intro_presents:I

    invoke-virtual {v2, v3}, Landroid/app/Activity;->getString(I)Ljava/lang/String;

    move-result-object v2

    const/16 v3, 0xd2

    invoke-static {v3, v14, v14, v14}, Landroid/graphics/Color;->argb(IIII)I

    move-result v3

    const/4 v4, 0x0

    const v5, 0x3eb33333    # 0.35f

    move-object/from16 v22, v1

    move-object v1, v2

    .end local v1    # "studio":Landroid/widget/TextView;
    .local v22, "studio":Landroid/widget/TextView;
    const/high16 v2, 0x41200000    # 10.0f

    invoke-direct/range {v0 .. v5}, Lcom/gta/launcher/ui/IntroOverlay;->text(Ljava/lang/String;FIZF)Landroid/widget/TextView;

    move-result-object v1

    iput-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->presents:Landroid/widget/TextView;

    .line 190
    iget-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->presents:Landroid/widget/TextView;

    invoke-virtual {v1, v8}, Landroid/widget/TextView;->setAlpha(F)V

    .line 191
    new-instance v1, Landroid/widget/LinearLayout$LayoutParams;

    invoke-direct {v1, v11, v11}, Landroid/widget/LinearLayout$LayoutParams;-><init>(II)V

    .line 192
    .local v1, "pp":Landroid/widget/LinearLayout$LayoutParams;
    invoke-direct {v0, v12}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v2

    float-to-int v2, v2

    iput v2, v1, Landroid/widget/LinearLayout$LayoutParams;->topMargin:I

    .line 193
    iget-object v2, v0, Lcom/gta/launcher/ui/IntroOverlay;->studioBox:Landroid/widget/LinearLayout;

    iget-object v3, v0, Lcom/gta/launcher/ui/IntroOverlay;->presents:Landroid/widget/TextView;

    invoke-virtual {v2, v3, v1}, Landroid/widget/LinearLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 194
    new-instance v2, Landroid/widget/FrameLayout$LayoutParams;

    const/16 v12, 0x31

    invoke-direct {v2, v11, v11, v12}, Landroid/widget/FrameLayout$LayoutParams;-><init>(III)V

    .line 195
    .local v2, "sp":Landroid/widget/FrameLayout$LayoutParams;
    const/high16 v3, 0x42000000    # 32.0f

    invoke-direct {v0, v3}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v3

    float-to-int v3, v3

    iput v3, v2, Landroid/widget/FrameLayout$LayoutParams;->topMargin:I

    .line 196
    iget-object v3, v0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->studioBox:Landroid/widget/LinearLayout;

    invoke-virtual {v3, v4, v2}, Landroid/widget/FrameLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 199
    new-instance v3, Landroid/widget/LinearLayout;

    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    invoke-direct {v3, v4}, Landroid/widget/LinearLayout;-><init>(Landroid/content/Context;)V

    .line 200
    .local v3, "mark":Landroid/widget/LinearLayout;
    invoke-virtual {v3, v6}, Landroid/widget/LinearLayout;->setOrientation(I)V

    .line 201
    invoke-virtual {v3, v6}, Landroid/widget/LinearLayout;->setGravity(I)V

    .line 202
    invoke-virtual {v3, v8}, Landroid/widget/LinearLayout;->setAlpha(F)V

    .line 203
    new-instance v4, Landroid/widget/LinearLayout;

    iget-object v5, v0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    invoke-direct {v4, v5}, Landroid/widget/LinearLayout;-><init>(Landroid/content/Context;)V

    .line 204
    .local v4, "markRow":Landroid/widget/LinearLayout;
    invoke-virtual {v4, v7}, Landroid/widget/LinearLayout;->setOrientation(I)V

    .line 205
    const/16 v5, 0x10

    invoke-virtual {v4, v5}, Landroid/widget/LinearLayout;->setGravity(I)V

    .line 206
    new-instance v5, Landroid/view/View;

    iget-object v7, v0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    invoke-direct {v5, v7}, Landroid/view/View;-><init>(Landroid/content/Context;)V

    move-object v7, v5

    .line 207
    .local v7, "ruleL":Landroid/view/View;
    new-instance v5, Landroid/graphics/drawable/GradientDrawable;

    sget-object v8, Landroid/graphics/drawable/GradientDrawable$Orientation;->LEFT_RIGHT:Landroid/graphics/drawable/GradientDrawable$Orientation;

    const v9, 0x3b82ff

    const v14, -0xc47d01

    filled-new-array {v9, v14}, [I

    move-result-object v12

    invoke-direct {v5, v8, v12}, Landroid/graphics/drawable/GradientDrawable;-><init>(Landroid/graphics/drawable/GradientDrawable$Orientation;[I)V

    invoke-virtual {v7, v5}, Landroid/view/View;->setBackground(Landroid/graphics/drawable/Drawable;)V

    .line 208
    new-instance v5, Landroid/widget/LinearLayout$LayoutParams;

    const/high16 v8, 0x42600000    # 56.0f

    invoke-direct {v0, v8}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v12

    float-to-int v12, v12

    const/high16 v8, 0x3f800000    # 1.0f

    invoke-direct {v0, v8}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v9

    float-to-int v9, v9

    invoke-static {v6, v9}, Ljava/lang/Math;->max(II)I

    move-result v9

    invoke-direct {v5, v12, v9}, Landroid/widget/LinearLayout$LayoutParams;-><init>(II)V

    invoke-virtual {v4, v7, v5}, Landroid/widget/LinearLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 209
    move-object v5, v4

    .end local v4    # "markRow":Landroid/widget/LinearLayout;
    .local v5, "markRow":Landroid/widget/LinearLayout;
    const/4 v4, 0x1

    move-object v9, v5

    .end local v5    # "markRow":Landroid/widget/LinearLayout;
    .local v9, "markRow":Landroid/widget/LinearLayout;
    const v5, 0x3ef5c28f    # 0.48f

    move-object v12, v1

    .end local v1    # "pp":Landroid/widget/LinearLayout$LayoutParams;
    .local v12, "pp":Landroid/widget/LinearLayout$LayoutParams;
    const-string v1, "CRESTWOOD"

    move-object/from16 v27, v2

    .end local v2    # "sp":Landroid/widget/FrameLayout$LayoutParams;
    .local v27, "sp":Landroid/widget/FrameLayout$LayoutParams;
    const/high16 v2, 0x41b00000    # 22.0f

    move-object/from16 v28, v3

    .end local v3    # "mark":Landroid/widget/LinearLayout;
    .local v28, "mark":Landroid/widget/LinearLayout;
    const/4 v3, -0x1

    move-object v6, v9

    move-object/from16 v9, v28

    .end local v28    # "mark":Landroid/widget/LinearLayout;
    .local v6, "markRow":Landroid/widget/LinearLayout;
    .local v9, "mark":Landroid/widget/LinearLayout;
    invoke-direct/range {v0 .. v5}, Lcom/gta/launcher/ui/IntroOverlay;->text(Ljava/lang/String;FIZF)Landroid/widget/TextView;

    move-result-object v1

    .line 210
    .local v1, "markWord":Landroid/widget/TextView;
    new-instance v2, Landroid/widget/LinearLayout$LayoutParams;

    invoke-direct {v2, v11, v11}, Landroid/widget/LinearLayout$LayoutParams;-><init>(II)V

    .line 211
    .local v2, "mwl":Landroid/widget/LinearLayout$LayoutParams;
    const/high16 v3, 0x41600000    # 14.0f

    invoke-direct {v0, v3}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v3

    float-to-int v3, v3

    iput v3, v2, Landroid/widget/LinearLayout$LayoutParams;->leftMargin:I

    .line 212
    const/high16 v3, 0x40800000    # 4.0f

    invoke-direct {v0, v3}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v3

    float-to-int v3, v3

    iput v3, v2, Landroid/widget/LinearLayout$LayoutParams;->rightMargin:I

    .line 213
    invoke-virtual {v6, v1, v2}, Landroid/widget/LinearLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 214
    new-instance v3, Landroid/view/View;

    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    invoke-direct {v3, v4}, Landroid/view/View;-><init>(Landroid/content/Context;)V

    .line 215
    .local v3, "ruleR":Landroid/view/View;
    new-instance v4, Landroid/graphics/drawable/GradientDrawable;

    sget-object v5, Landroid/graphics/drawable/GradientDrawable$Orientation;->LEFT_RIGHT:Landroid/graphics/drawable/GradientDrawable$Orientation;

    const v11, 0x3b82ff

    filled-new-array {v14, v11}, [I

    move-result-object v11

    invoke-direct {v4, v5, v11}, Landroid/graphics/drawable/GradientDrawable;-><init>(Landroid/graphics/drawable/GradientDrawable$Orientation;[I)V

    invoke-virtual {v3, v4}, Landroid/view/View;->setBackground(Landroid/graphics/drawable/Drawable;)V

    .line 216
    new-instance v4, Landroid/widget/LinearLayout$LayoutParams;

    const/high16 v5, 0x42600000    # 56.0f

    invoke-direct {v0, v5}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v5

    float-to-int v5, v5

    invoke-direct {v0, v8}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v8

    float-to-int v8, v8

    const/4 v11, 0x1

    invoke-static {v11, v8}, Ljava/lang/Math;->max(II)I

    move-result v8

    invoke-direct {v4, v5, v8}, Landroid/widget/LinearLayout$LayoutParams;-><init>(II)V

    invoke-virtual {v6, v3, v4}, Landroid/widget/LinearLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 217
    invoke-virtual {v9, v6}, Landroid/widget/LinearLayout;->addView(Landroid/view/View;)V

    .line 218
    const/4 v4, 0x1

    const/high16 v5, 0x3f000000    # 0.5f

    move-object v8, v1

    .end local v1    # "markWord":Landroid/widget/TextView;
    .local v8, "markWord":Landroid/widget/TextView;
    const-string v1, "ROLEPLAY  \u00b7  SAISON 1"

    move-object v11, v2

    .end local v2    # "mwl":Landroid/widget/LinearLayout$LayoutParams;
    .local v11, "mwl":Landroid/widget/LinearLayout$LayoutParams;
    const/high16 v2, 0x41080000    # 8.5f

    move-object v14, v3

    .end local v3    # "ruleR":Landroid/view/View;
    .local v14, "ruleR":Landroid/view/View;
    const v3, -0xc47d01

    invoke-direct/range {v0 .. v5}, Lcom/gta/launcher/ui/IntroOverlay;->text(Ljava/lang/String;FIZF)Landroid/widget/TextView;

    move-result-object v1

    .line 219
    .local v1, "markSub":Landroid/widget/TextView;
    new-instance v2, Landroid/widget/LinearLayout$LayoutParams;

    const/4 v3, -0x2

    invoke-direct {v2, v3, v3}, Landroid/widget/LinearLayout$LayoutParams;-><init>(II)V

    .line 220
    .local v2, "msl":Landroid/widget/LinearLayout$LayoutParams;
    const/high16 v4, 0x40c00000    # 6.0f

    invoke-direct {v0, v4}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v4

    float-to-int v4, v4

    iput v4, v2, Landroid/widget/LinearLayout$LayoutParams;->topMargin:I

    .line 221
    invoke-virtual {v9, v1, v2}, Landroid/widget/LinearLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 222
    new-instance v4, Landroid/widget/FrameLayout$LayoutParams;

    const/16 v5, 0x31

    invoke-direct {v4, v3, v3, v5}, Landroid/widget/FrameLayout$LayoutParams;-><init>(III)V

    move-object v3, v4

    .line 223
    .local v3, "mlp":Landroid/widget/FrameLayout$LayoutParams;
    const/high16 v4, 0x42100000    # 36.0f

    invoke-direct {v0, v4}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v4

    float-to-int v4, v4

    iput v4, v3, Landroid/widget/FrameLayout$LayoutParams;->topMargin:I

    .line 224
    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    invoke-virtual {v4, v9, v3}, Landroid/widget/FrameLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 225
    iput-object v9, v0, Lcom/gta/launcher/ui/IntroOverlay;->topMark:Landroid/view/View;

    .line 227
    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    sget v5, Lcom/gta/game/R$string;->intro_copyright:I

    invoke-virtual {v4, v5}, Landroid/app/Activity;->getString(I)Ljava/lang/String;

    move-result-object v4

    move-object/from16 v24, v3

    const/16 v0, 0xff

    const/16 v5, 0x96

    .end local v3    # "mlp":Landroid/widget/FrameLayout$LayoutParams;
    .local v24, "mlp":Landroid/widget/FrameLayout$LayoutParams;
    invoke-static {v5, v0, v0, v0}, Landroid/graphics/Color;->argb(IIII)I

    move-result v3

    move-object v0, v1

    move-object v1, v4

    .end local v1    # "markSub":Landroid/widget/TextView;
    .local v0, "markSub":Landroid/widget/TextView;
    const/4 v4, 0x0

    const v5, 0x3da3d70a    # 0.08f

    move-object/from16 v25, v2

    .end local v2    # "msl":Landroid/widget/LinearLayout$LayoutParams;
    .local v25, "msl":Landroid/widget/LinearLayout$LayoutParams;
    const/high16 v2, 0x41080000    # 8.5f

    move-object/from16 v26, v24

    move-object/from16 v24, v0

    move-object/from16 v0, p0

    .end local v0    # "markSub":Landroid/widget/TextView;
    .local v24, "markSub":Landroid/widget/TextView;
    .local v26, "mlp":Landroid/widget/FrameLayout$LayoutParams;
    invoke-direct/range {v0 .. v5}, Lcom/gta/launcher/ui/IntroOverlay;->text(Ljava/lang/String;FIZF)Landroid/widget/TextView;

    move-result-object v1

    iput-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->copyright:Landroid/widget/TextView;

    .line 228
    iget-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->copyright:Landroid/widget/TextView;

    const/4 v2, 0x0

    invoke-virtual {v1, v2}, Landroid/widget/TextView;->setAlpha(F)V

    .line 229
    new-instance v1, Landroid/widget/FrameLayout$LayoutParams;

    const/16 v2, 0x51

    const/4 v3, -0x2

    invoke-direct {v1, v3, v3, v2}, Landroid/widget/FrameLayout$LayoutParams;-><init>(III)V

    .line 230
    .local v1, "crp":Landroid/widget/FrameLayout$LayoutParams;
    const/high16 v2, 0x41f00000    # 30.0f

    invoke-direct {v0, v2}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v3

    float-to-int v3, v3

    iput v3, v1, Landroid/widget/FrameLayout$LayoutParams;->bottomMargin:I

    .line 231
    iget-object v3, v0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->copyright:Landroid/widget/TextView;

    invoke-virtual {v3, v4, v1}, Landroid/widget/FrameLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 233
    iget-object v3, v0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    sget v4, Lcom/gta/game/R$string;->intro_skip:I

    invoke-virtual {v3, v4}, Landroid/app/Activity;->getString(I)Ljava/lang/String;

    move-result-object v3

    const/16 v4, 0xff

    const/16 v5, 0x96

    invoke-static {v5, v4, v4, v4}, Landroid/graphics/Color;->argb(IIII)I

    move-result v5

    const/4 v4, 0x0

    move-object/from16 v23, v1

    move-object v1, v3

    move v3, v5

    .end local v1    # "crp":Landroid/widget/FrameLayout$LayoutParams;
    .local v23, "crp":Landroid/widget/FrameLayout$LayoutParams;
    const v5, 0x3df5c28f    # 0.12f

    move/from16 v28, v2

    const/high16 v2, 0x41100000    # 9.0f

    move/from16 v29, v28

    move-object/from16 v28, v6

    move/from16 v6, v29

    .end local v6    # "markRow":Landroid/widget/LinearLayout;
    .local v28, "markRow":Landroid/widget/LinearLayout;
    invoke-direct/range {v0 .. v5}, Lcom/gta/launcher/ui/IntroOverlay;->text(Ljava/lang/String;FIZF)Landroid/widget/TextView;

    move-result-object v1

    iput-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->skipHint:Landroid/widget/TextView;

    .line 234
    iget-object v1, v0, Lcom/gta/launcher/ui/IntroOverlay;->skipHint:Landroid/widget/TextView;

    const/4 v2, 0x0

    invoke-virtual {v1, v2}, Landroid/widget/TextView;->setAlpha(F)V

    .line 235
    new-instance v1, Landroid/widget/FrameLayout$LayoutParams;

    const v2, 0x800055

    const/4 v3, -0x2

    invoke-direct {v1, v3, v3, v2}, Landroid/widget/FrameLayout$LayoutParams;-><init>(III)V

    .line 236
    .local v1, "skp":Landroid/widget/FrameLayout$LayoutParams;
    invoke-direct {v0, v6}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v2

    float-to-int v2, v2

    iput v2, v1, Landroid/widget/FrameLayout$LayoutParams;->rightMargin:I

    .line 237
    invoke-direct {v0, v6}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v2

    float-to-int v2, v2

    iput v2, v1, Landroid/widget/FrameLayout$LayoutParams;->bottomMargin:I

    .line 238
    iget-object v2, v0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    iget-object v3, v0, Lcom/gta/launcher/ui/IntroOverlay;->skipHint:Landroid/widget/TextView;

    invoke-virtual {v2, v3, v1}, Landroid/widget/FrameLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 241
    new-instance v2, Landroid/widget/FrameLayout;

    iget-object v3, v0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    invoke-direct {v2, v3}, Landroid/widget/FrameLayout;-><init>(Landroid/content/Context;)V

    .line 242
    .local v2, "track":Landroid/widget/FrameLayout;
    const/16 v3, 0x28

    const/16 v4, 0xff

    invoke-static {v3, v4, v4, v4}, Landroid/graphics/Color;->argb(IIII)I

    move-result v3

    invoke-virtual {v2, v3}, Landroid/widget/FrameLayout;->setBackgroundColor(I)V

    .line 243
    new-instance v3, Landroid/widget/FrameLayout$LayoutParams;

    const/high16 v4, 0x3fc00000    # 1.5f

    invoke-direct {v0, v4}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v4

    float-to-int v4, v4

    const/4 v5, 0x2

    invoke-static {v5, v4}, Ljava/lang/Math;->max(II)I

    move-result v4

    const/16 v5, 0x50

    const/4 v6, -0x1

    invoke-direct {v3, v6, v4, v5}, Landroid/widget/FrameLayout$LayoutParams;-><init>(III)V

    .line 244
    .local v3, "tp":Landroid/widget/FrameLayout$LayoutParams;
    const/high16 v4, 0x41b00000    # 22.0f

    invoke-direct {v0, v4}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v4

    float-to-int v4, v4

    iput v4, v3, Landroid/widget/FrameLayout$LayoutParams;->bottomMargin:I

    .line 245
    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    invoke-virtual {v4, v2, v3}, Landroid/widget/FrameLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 246
    new-instance v4, Landroid/view/View;

    iget-object v5, v0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    invoke-direct {v4, v5}, Landroid/view/View;-><init>(Landroid/content/Context;)V

    iput-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->progressFill:Landroid/view/View;

    .line 247
    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->progressFill:Landroid/view/View;

    const/16 v5, 0xe6

    const/16 v6, 0xff

    invoke-static {v5, v6, v6, v6}, Landroid/graphics/Color;->argb(IIII)I

    move-result v5

    invoke-virtual {v4, v5}, Landroid/view/View;->setBackgroundColor(I)V

    .line 248
    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->progressFill:Landroid/view/View;

    const/4 v5, 0x0

    invoke-virtual {v4, v5}, Landroid/view/View;->setPivotX(F)V

    .line 249
    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->progressFill:Landroid/view/View;

    invoke-virtual {v4, v5}, Landroid/view/View;->setScaleX(F)V

    .line 250
    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->progressFill:Landroid/view/View;

    new-instance v5, Landroid/widget/FrameLayout$LayoutParams;

    const/4 v6, -0x1

    invoke-direct {v5, v6, v6}, Landroid/widget/FrameLayout$LayoutParams;-><init>(II)V

    invoke-virtual {v2, v4, v5}, Landroid/widget/FrameLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 253
    const/4 v4, 0x0

    .local v4, "i":I
    :goto_0
    iget-object v5, v0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    invoke-virtual {v5}, Landroid/widget/FrameLayout;->getChildCount()I

    move-result v5

    if-ge v4, v5, :cond_1

    .line 254
    iget-object v5, v0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    invoke-virtual {v5, v4}, Landroid/widget/FrameLayout;->getChildAt(I)Landroid/view/View;

    move-result-object v5

    .line 255
    .local v5, "child":Landroid/view/View;
    iget-object v6, v0, Lcom/gta/launcher/ui/IntroOverlay;->front:Landroid/widget/ImageView;

    if-eq v5, v6, :cond_0

    iget-object v6, v0, Lcom/gta/launcher/ui/IntroOverlay;->back:Landroid/widget/ImageView;

    if-eq v5, v6, :cond_0

    const/4 v6, 0x0

    invoke-virtual {v5, v6}, Landroid/view/View;->setOutlineProvider(Landroid/view/ViewOutlineProvider;)V

    const/high16 v6, 0x41200000    # 10.0f

    invoke-virtual {v5, v6}, Landroid/view/View;->setTranslationZ(F)V

    .line 253
    .end local v5    # "child":Landroid/view/View;
    :cond_0
    add-int/lit8 v4, v4, 0x1

    goto :goto_0

    .line 258
    .end local v4    # "i":I
    :cond_1
    iget-object v4, v0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    iget-object v5, v0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    new-instance v6, Landroid/view/ViewGroup$LayoutParams;

    const/4 v0, -0x1

    invoke-direct {v6, v0, v0}, Landroid/view/ViewGroup$LayoutParams;-><init>(II)V

    invoke-virtual {v4, v5, v6}, Landroid/app/Activity;->addContentView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 260
    return-void
.end method

.method private dp(F)F
    .locals 2
    .param p1, "value"    # F

    .line 99
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    .line 100
    invoke-virtual {v0}, Landroid/app/Activity;->getResources()Landroid/content/res/Resources;

    move-result-object v0

    invoke-virtual {v0}, Landroid/content/res/Resources;->getDisplayMetrics()Landroid/util/DisplayMetrics;

    move-result-object v0

    .line 99
    const/4 v1, 0x1

    invoke-static {v1, p1, v0}, Landroid/util/TypedValue;->applyDimension(IFLandroid/util/DisplayMetrics;)F

    move-result v0

    return v0
.end method

.method private fadeMusic(J)V
    .locals 7
    .param p1, "durationMs"    # J

    .line 384
    iget-object v2, p0, Lcom/gta/launcher/ui/IntroOverlay;->player:Landroid/media/MediaPlayer;

    .line 385
    .local v2, "current":Landroid/media/MediaPlayer;
    if-nez v2, :cond_0

    return-void

    .line 386
    :cond_0
    invoke-static {}, Ljava/lang/System;->currentTimeMillis()J

    move-result-wide v3

    .line 387
    .local v3, "fadeStart":J
    new-instance v0, Lcom/gta/launcher/ui/IntroOverlay$1;

    move-object v1, p0

    move-wide v5, p1

    .end local p1    # "durationMs":J
    .local v5, "durationMs":J
    invoke-direct/range {v0 .. v6}, Lcom/gta/launcher/ui/IntroOverlay$1;-><init>(Lcom/gta/launcher/ui/IntroOverlay;Landroid/media/MediaPlayer;JJ)V

    .line 396
    .local v0, "step":Ljava/lang/Runnable;
    iget-object p1, v1, Lcom/gta/launcher/ui/IntroOverlay;->handler:Landroid/os/Handler;

    invoke-virtual {p1, v0}, Landroid/os/Handler;->post(Ljava/lang/Runnable;)Z

    .line 397
    return-void
.end method

.method private fadeOutAndFinish(J)V
    .locals 2
    .param p1, "durationMs"    # J

    .line 377
    iget-boolean v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->finished:Z

    if-nez v0, :cond_1

    iget-boolean v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->fadingOut:Z

    if-eqz v0, :cond_0

    goto :goto_0

    .line 378
    :cond_0
    const/4 v0, 0x1

    iput-boolean v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->fadingOut:Z

    .line 379
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    invoke-virtual {v0}, Landroid/widget/FrameLayout;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const/4 v1, 0x0

    invoke-virtual {v0, v1}, Landroid/view/ViewPropertyAnimator;->alpha(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    invoke-virtual {v0, p1, p2}, Landroid/view/ViewPropertyAnimator;->setDuration(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    new-instance v1, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda6;

    invoke-direct {v1, p0}, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda6;-><init>(Lcom/gta/launcher/ui/IntroOverlay;)V

    invoke-virtual {v0, v1}, Landroid/view/ViewPropertyAnimator;->withEndAction(Ljava/lang/Runnable;)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    invoke-virtual {v0}, Landroid/view/ViewPropertyAnimator;->start()V

    .line 380
    invoke-direct {p0, p1, p2}, Lcom/gta/launcher/ui/IntroOverlay;->fadeMusic(J)V

    .line 381
    return-void

    .line 377
    :cond_1
    :goto_0
    return-void
.end method

.method private finish()V
    .locals 2

    .line 400
    iget-boolean v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->finished:Z

    if-eqz v0, :cond_0

    return-void

    .line 401
    :cond_0
    const/4 v0, 0x1

    iput-boolean v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->finished:Z

    .line 402
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->handler:Landroid/os/Handler;

    const/4 v1, 0x0

    invoke-virtual {v0, v1}, Landroid/os/Handler;->removeCallbacksAndMessages(Ljava/lang/Object;)V

    .line 403
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->fadeAnimator:Landroid/animation/ObjectAnimator;

    if-eqz v0, :cond_1

    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->fadeAnimator:Landroid/animation/ObjectAnimator;

    invoke-virtual {v0}, Landroid/animation/ObjectAnimator;->cancel()V

    .line 404
    :cond_1
    invoke-direct {p0}, Lcom/gta/launcher/ui/IntroOverlay;->releasePlayer()V

    .line 406
    :try_start_0
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    if-nez v0, :cond_2

    :goto_0
    goto :goto_1

    :cond_2
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    invoke-virtual {v0}, Landroid/widget/FrameLayout;->getParent()Landroid/view/ViewParent;

    move-result-object v0

    move-object v1, v0

    check-cast v1, Landroid/view/ViewGroup;

    goto :goto_0

    .line 407
    .local v1, "parent":Landroid/view/ViewGroup;
    :goto_1
    if-eqz v1, :cond_3

    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    invoke-virtual {v1, v0}, Landroid/view/ViewGroup;->removeView(Landroid/view/View;)V
    :try_end_0
    .catchall {:try_start_0 .. :try_end_0} :catchall_0

    goto :goto_2

    .line 408
    .end local v1    # "parent":Landroid/view/ViewGroup;
    :catchall_0
    move-exception v0

    :cond_3
    :goto_2
    nop

    .line 409
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->onFinished:Ljava/lang/Runnable;

    if-eqz v0, :cond_4

    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->onFinished:Ljava/lang/Runnable;

    invoke-interface {v0}, Ljava/lang/Runnable;->run()V

    .line 410
    :cond_4
    return-void
.end method

.method private varargs gradient(IILandroid/graphics/drawable/GradientDrawable$Orientation;[I)Landroid/view/View;
    .locals 5
    .param p1, "gravity"    # I
    .param p2, "heightDp"    # I
    .param p3, "orientation"    # Landroid/graphics/drawable/GradientDrawable$Orientation;
    .param p4, "colors"    # [I

    .line 114
    new-instance v0, Landroid/view/View;

    iget-object v1, p0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    invoke-direct {v0, v1}, Landroid/view/View;-><init>(Landroid/content/Context;)V

    .line 115
    .local v0, "view":Landroid/view/View;
    new-instance v1, Landroid/graphics/drawable/GradientDrawable;

    invoke-direct {v1, p3, p4}, Landroid/graphics/drawable/GradientDrawable;-><init>(Landroid/graphics/drawable/GradientDrawable$Orientation;[I)V

    invoke-virtual {v0, v1}, Landroid/view/View;->setBackground(Landroid/graphics/drawable/Drawable;)V

    .line 116
    iget-object v1, p0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    new-instance v2, Landroid/widget/FrameLayout$LayoutParams;

    .line 117
    const/4 v3, -0x1

    if-gez p2, :cond_0

    move v4, v3

    goto :goto_0

    :cond_0
    int-to-float v4, p2

    invoke-direct {p0, v4}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v4

    float-to-int v4, v4

    :goto_0
    invoke-direct {v2, v3, v4, p1}, Landroid/widget/FrameLayout$LayoutParams;-><init>(III)V

    .line 116
    invoke-virtual {v1, v0, v2}, Landroid/widget/FrameLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 118
    return-object v0
.end method

.method private synthetic lambda$build$0(Landroid/view/View;)V
    .locals 2
    .param p1, "view"    # Landroid/view/View;

    .line 138
    iget-boolean v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->skipAllowed:Z

    if-eqz v0, :cond_0

    const-wide/16 v0, 0x1c2

    invoke-direct {p0, v0, v1}, Lcom/gta/launcher/ui/IntroOverlay;->fadeOutAndFinish(J)V

    :cond_0
    return-void
.end method

.method private synthetic lambda$showCaption$4()V
    .locals 5

    .line 339
    iget-boolean v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->finished:Z

    if-eqz v0, :cond_0

    return-void

    .line 340
    :cond_0
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->captionBox:Landroid/widget/LinearLayout;

    invoke-virtual {v0}, Landroid/widget/LinearLayout;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const/4 v1, 0x0

    invoke-virtual {v0, v1}, Landroid/view/ViewPropertyAnimator;->alpha(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const-wide/16 v1, 0x0

    const-wide/16 v3, 0x898

    invoke-static {v1, v2, v3, v4}, Ljava/lang/Math;->max(JJ)J

    move-result-wide v1

    invoke-virtual {v0, v1, v2}, Landroid/view/ViewPropertyAnimator;->setStartDelay(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const-wide/16 v1, 0x258

    invoke-virtual {v0, v1, v2}, Landroid/view/ViewPropertyAnimator;->setDuration(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    invoke-virtual {v0}, Landroid/view/ViewPropertyAnimator;->start()V

    .line 341
    return-void
.end method

.method private synthetic lambda$showCaption$5([Ljava/lang/String;)V
    .locals 3
    .param p1, "caption"    # [Ljava/lang/String;

    .line 333
    iget-boolean v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->finished:Z

    if-eqz v0, :cond_0

    return-void

    .line 334
    :cond_0
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->captionKicker:Landroid/widget/TextView;

    const/4 v1, 0x0

    aget-object v1, p1, v1

    invoke-virtual {v0, v1}, Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V

    .line 335
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->captionLine:Landroid/widget/TextView;

    const/4 v1, 0x1

    aget-object v1, p1, v1

    invoke-virtual {v0, v1}, Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V

    .line 336
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->captionBox:Landroid/widget/LinearLayout;

    const/high16 v1, 0x41900000    # 18.0f

    invoke-direct {p0, v1}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v1

    invoke-virtual {v0, v1}, Landroid/widget/LinearLayout;->setTranslationX(F)V

    .line 337
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->captionBox:Landroid/widget/LinearLayout;

    invoke-virtual {v0}, Landroid/widget/LinearLayout;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const/high16 v1, 0x3f800000    # 1.0f

    invoke-virtual {v0, v1}, Landroid/view/ViewPropertyAnimator;->alpha(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const/4 v1, 0x0

    invoke-virtual {v0, v1}, Landroid/view/ViewPropertyAnimator;->translationX(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const-wide/16 v1, 0x1c2

    invoke-virtual {v0, v1, v2}, Landroid/view/ViewPropertyAnimator;->setStartDelay(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const-wide/16 v1, 0x384

    invoke-virtual {v0, v1, v2}, Landroid/view/ViewPropertyAnimator;->setDuration(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    new-instance v1, Landroid/view/animation/AccelerateDecelerateInterpolator;

    invoke-direct {v1}, Landroid/view/animation/AccelerateDecelerateInterpolator;-><init>()V

    .line 338
    invoke-virtual {v0, v1}, Landroid/view/ViewPropertyAnimator;->setInterpolator(Landroid/animation/TimeInterpolator;)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    new-instance v1, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda4;

    invoke-direct {v1, p0}, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda4;-><init>(Lcom/gta/launcher/ui/IntroOverlay;)V

    invoke-virtual {v0, v1}, Landroid/view/ViewPropertyAnimator;->withEndAction(Ljava/lang/Runnable;)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    .line 341
    invoke-virtual {v0}, Landroid/view/ViewPropertyAnimator;->start()V

    .line 342
    return-void
.end method

.method private synthetic lambda$showFinale$6()V
    .locals 4

    .line 353
    iget-boolean v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->finished:Z

    if-eqz v0, :cond_0

    return-void

    .line 354
    :cond_0
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->studioBox:Landroid/widget/LinearLayout;

    invoke-virtual {v0}, Landroid/widget/LinearLayout;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const/high16 v1, 0x3f800000    # 1.0f

    invoke-virtual {v0, v1}, Landroid/view/ViewPropertyAnimator;->alpha(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const-wide/16 v2, 0x3e8

    invoke-virtual {v0, v2, v3}, Landroid/view/ViewPropertyAnimator;->setDuration(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    invoke-virtual {v0}, Landroid/view/ViewPropertyAnimator;->start()V

    .line 355
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->copyright:Landroid/widget/TextView;

    invoke-virtual {v0}, Landroid/widget/TextView;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    invoke-virtual {v0, v1}, Landroid/view/ViewPropertyAnimator;->alpha(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    invoke-virtual {v0, v2, v3}, Landroid/view/ViewPropertyAnimator;->setDuration(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    invoke-virtual {v0}, Landroid/view/ViewPropertyAnimator;->start()V

    .line 356
    return-void
.end method

.method private synthetic lambda$showFinale$7()V
    .locals 3

    .line 358
    iget-boolean v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->finished:Z

    if-nez v0, :cond_0

    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->presents:Landroid/widget/TextView;

    invoke-virtual {v0}, Landroid/widget/TextView;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const/high16 v1, 0x3f800000    # 1.0f

    invoke-virtual {v0, v1}, Landroid/view/ViewPropertyAnimator;->alpha(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const-wide/16 v1, 0x384

    invoke-virtual {v0, v1, v2}, Landroid/view/ViewPropertyAnimator;->setDuration(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    invoke-virtual {v0}, Landroid/view/ViewPropertyAnimator;->start()V

    .line 359
    :cond_0
    return-void
.end method

.method private synthetic lambda$start$1(I)V
    .locals 2
    .param p1, "index"    # I

    .line 275
    sget-object v0, Lcom/gta/launcher/ui/IntroOverlay;->SLIDES:[I

    sget-object v1, Lcom/gta/launcher/ui/IntroOverlay;->SLIDES:[I

    array-length v1, v1

    rem-int v1, p1, v1

    aget v0, v0, v1

    const/4 v1, 0x0

    invoke-direct {p0, v0, p1, v1}, Lcom/gta/launcher/ui/IntroOverlay;->showSlide(IIZ)V

    return-void
.end method

.method private synthetic lambda$start$2()V
    .locals 3

    .line 280
    iget-boolean v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->finished:Z

    if-eqz v0, :cond_0

    return-void

    .line 281
    :cond_0
    const/4 v0, 0x1

    iput-boolean v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->skipAllowed:Z

    .line 282
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->skipHint:Landroid/widget/TextView;

    invoke-virtual {v0}, Landroid/widget/TextView;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const/high16 v1, 0x3f800000    # 1.0f

    invoke-virtual {v0, v1}, Landroid/view/ViewPropertyAnimator;->alpha(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const-wide/16 v1, 0x258

    invoke-virtual {v0, v1, v2}, Landroid/view/ViewPropertyAnimator;->setDuration(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    invoke-virtual {v0}, Landroid/view/ViewPropertyAnimator;->start()V

    .line 283
    return-void
.end method

.method private synthetic lambda$start$3()V
    .locals 2

    .line 285
    const-wide/16 v0, 0x578

    invoke-direct {p0, v0, v1}, Lcom/gta/launcher/ui/IntroOverlay;->fadeOutAndFinish(J)V

    return-void
.end method

.method private newPhoto()Landroid/widget/ImageView;
    .locals 4

    .line 104
    new-instance v0, Landroid/widget/ImageView;

    iget-object v1, p0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    invoke-direct {v0, v1}, Landroid/widget/ImageView;-><init>(Landroid/content/Context;)V

    .line 105
    .local v0, "view":Landroid/widget/ImageView;
    sget-object v1, Landroid/widget/ImageView$ScaleType;->CENTER_CROP:Landroid/widget/ImageView$ScaleType;

    invoke-virtual {v0, v1}, Landroid/widget/ImageView;->setScaleType(Landroid/widget/ImageView$ScaleType;)V

    .line 106
    const/4 v1, 0x0

    invoke-virtual {v0, v1}, Landroid/widget/ImageView;->setAlpha(F)V

    .line 107
    const/4 v1, 0x2

    invoke-virtual {v0, v1}, Landroid/widget/ImageView;->setImportantForAccessibility(I)V

    .line 108
    iget-object v1, p0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    new-instance v2, Landroid/widget/FrameLayout$LayoutParams;

    const/4 v3, -0x1

    invoke-direct {v2, v3, v3}, Landroid/widget/FrameLayout$LayoutParams;-><init>(II)V

    invoke-virtual {v1, v0, v2}, Landroid/widget/FrameLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V

    .line 110
    return-object v0
.end method

.method private openCinemaBars()V
    .locals 7

    .line 323
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    const-string v1, "barTop"

    invoke-virtual {v0, v1}, Landroid/widget/FrameLayout;->findViewWithTag(Ljava/lang/Object;)Landroid/view/View;

    move-result-object v0

    .line 324
    .local v0, "top":Landroid/view/View;
    iget-object v1, p0, Lcom/gta/launcher/ui/IntroOverlay;->root:Landroid/widget/FrameLayout;

    const-string v2, "barBottom"

    invoke-virtual {v1, v2}, Landroid/widget/FrameLayout;->findViewWithTag(Ljava/lang/Object;)Landroid/view/View;

    move-result-object v1

    .line 325
    .local v1, "bottom":Landroid/view/View;
    const-wide/16 v2, 0x708

    const/high16 v4, 0x3f800000    # 1.0f

    const v5, 0x4019999a    # 2.4f

    if-eqz v0, :cond_0

    const/4 v6, 0x0

    invoke-virtual {v0, v6}, Landroid/view/View;->setPivotY(F)V

    invoke-virtual {v0, v5}, Landroid/view/View;->setScaleY(F)V

    invoke-virtual {v0}, Landroid/view/View;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v6

    invoke-virtual {v6, v4}, Landroid/view/ViewPropertyAnimator;->scaleY(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v6

    invoke-virtual {v6, v2, v3}, Landroid/view/ViewPropertyAnimator;->setDuration(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v6

    invoke-virtual {v6}, Landroid/view/ViewPropertyAnimator;->start()V

    .line 326
    :cond_0
    if-eqz v1, :cond_1

    const/high16 v6, 0x41b00000    # 22.0f

    invoke-direct {p0, v6}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v6

    invoke-virtual {v1, v6}, Landroid/view/View;->setPivotY(F)V

    invoke-virtual {v1, v5}, Landroid/view/View;->setScaleY(F)V

    invoke-virtual {v1}, Landroid/view/View;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v5

    invoke-virtual {v5, v4}, Landroid/view/ViewPropertyAnimator;->scaleY(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v4

    invoke-virtual {v4, v2, v3}, Landroid/view/ViewPropertyAnimator;->setDuration(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v2

    invoke-virtual {v2}, Landroid/view/ViewPropertyAnimator;->start()V

    .line 327
    :cond_1
    return-void
.end method

.method private releasePlayer()V
    .locals 2

    .line 413
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->player:Landroid/media/MediaPlayer;

    .line 414
    .local v0, "current":Landroid/media/MediaPlayer;
    const/4 v1, 0x0

    iput-object v1, p0, Lcom/gta/launcher/ui/IntroOverlay;->player:Landroid/media/MediaPlayer;

    .line 415
    if-nez v0, :cond_0

    return-void

    .line 416
    :cond_0
    :try_start_0
    invoke-virtual {v0}, Landroid/media/MediaPlayer;->stop()V
    :try_end_0
    .catchall {:try_start_0 .. :try_end_0} :catchall_0

    goto :goto_0

    :catchall_0
    move-exception v1

    .line 417
    :goto_0
    :try_start_1
    invoke-virtual {v0}, Landroid/media/MediaPlayer;->release()V
    :try_end_1
    .catchall {:try_start_1 .. :try_end_1} :catchall_1

    goto :goto_1

    :catchall_1
    move-exception v1

    .line 418
    :goto_1
    return-void
.end method

.method private showCaption(I)V
    .locals 5
    .param p1, "index"    # I

    .line 330
    sget-object v0, Lcom/gta/launcher/ui/IntroOverlay;->CAPTIONS:[[Ljava/lang/String;

    sget-object v1, Lcom/gta/launcher/ui/IntroOverlay;->CAPTIONS:[[Ljava/lang/String;

    array-length v1, v1

    rem-int v1, p1, v1

    aget-object v0, v0, v1

    .line 331
    .local v0, "caption":[Ljava/lang/String;
    iget-object v1, p0, Lcom/gta/launcher/ui/IntroOverlay;->captionBox:Landroid/widget/LinearLayout;

    invoke-virtual {v1}, Landroid/widget/LinearLayout;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v1

    invoke-virtual {v1}, Landroid/view/ViewPropertyAnimator;->cancel()V

    .line 332
    iget-object v1, p0, Lcom/gta/launcher/ui/IntroOverlay;->captionBox:Landroid/widget/LinearLayout;

    invoke-virtual {v1}, Landroid/widget/LinearLayout;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v1

    const-wide/16 v2, 0x0

    invoke-virtual {v1, v2, v3}, Landroid/view/ViewPropertyAnimator;->setStartDelay(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v1

    const/4 v4, 0x0

    invoke-virtual {v1, v4}, Landroid/view/ViewPropertyAnimator;->alpha(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v1

    const/high16 v4, 0x41000000    # 8.0f

    invoke-direct {p0, v4}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v4

    neg-float v4, v4

    invoke-virtual {v1, v4}, Landroid/view/ViewPropertyAnimator;->translationX(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v1

    if-nez p1, :cond_0

    goto :goto_0

    :cond_0
    const-wide/16 v2, 0x15e

    :goto_0
    invoke-virtual {v1, v2, v3}, Landroid/view/ViewPropertyAnimator;->setDuration(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v1

    new-instance v2, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda9;

    invoke-direct {v2, p0, v0}, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda9;-><init>(Lcom/gta/launcher/ui/IntroOverlay;[Ljava/lang/String;)V

    invoke-virtual {v1, v2}, Landroid/view/ViewPropertyAnimator;->withEndAction(Ljava/lang/Runnable;)Landroid/view/ViewPropertyAnimator;

    move-result-object v1

    .line 342
    invoke-virtual {v1}, Landroid/view/ViewPropertyAnimator;->start()V

    .line 343
    return-void
.end method

.method private showFinale()V
    .locals 6

    .line 346
    iget-boolean v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->finished:Z

    if-eqz v0, :cond_0

    return-void

    .line 347
    :cond_0
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->captionBox:Landroid/widget/LinearLayout;

    invoke-virtual {v0}, Landroid/widget/LinearLayout;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    invoke-virtual {v0}, Landroid/view/ViewPropertyAnimator;->cancel()V

    .line 348
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->captionBox:Landroid/widget/LinearLayout;

    invoke-virtual {v0}, Landroid/widget/LinearLayout;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const-wide/16 v1, 0x0

    invoke-virtual {v0, v1, v2}, Landroid/view/ViewPropertyAnimator;->setStartDelay(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const/4 v3, 0x0

    invoke-virtual {v0, v3}, Landroid/view/ViewPropertyAnimator;->alpha(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const-wide/16 v4, 0x190

    invoke-virtual {v0, v4, v5}, Landroid/view/ViewPropertyAnimator;->setDuration(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    invoke-virtual {v0}, Landroid/view/ViewPropertyAnimator;->start()V

    .line 349
    sget v0, Lcom/gta/game/R$drawable;->crestwood_intro:I

    const/4 v4, 0x4

    const/4 v5, 0x1

    invoke-direct {p0, v0, v4, v5}, Lcom/gta/launcher/ui/IntroOverlay;->showSlide(IIZ)V

    .line 351
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->topMark:Landroid/view/View;

    if-eqz v0, :cond_1

    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->topMark:Landroid/view/View;

    invoke-virtual {v0}, Landroid/view/View;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    invoke-virtual {v0, v3}, Landroid/view/ViewPropertyAnimator;->alpha(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const/high16 v3, 0x41200000    # 10.0f

    invoke-direct {p0, v3}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v3

    neg-float v3, v3

    invoke-virtual {v0, v3}, Landroid/view/ViewPropertyAnimator;->translationY(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    invoke-virtual {v0, v1, v2}, Landroid/view/ViewPropertyAnimator;->setStartDelay(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const-wide/16 v1, 0x2bc

    invoke-virtual {v0, v1, v2}, Landroid/view/ViewPropertyAnimator;->setDuration(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    invoke-virtual {v0}, Landroid/view/ViewPropertyAnimator;->start()V

    .line 352
    :cond_1
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->handler:Landroid/os/Handler;

    new-instance v1, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda7;

    invoke-direct {v1, p0}, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda7;-><init>(Lcom/gta/launcher/ui/IntroOverlay;)V

    const-wide/16 v2, 0x384

    invoke-virtual {v0, v1, v2, v3}, Landroid/os/Handler;->postDelayed(Ljava/lang/Runnable;J)Z

    .line 357
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->handler:Landroid/os/Handler;

    new-instance v1, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda8;

    invoke-direct {v1, p0}, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda8;-><init>(Lcom/gta/launcher/ui/IntroOverlay;)V

    const-wide/16 v2, 0x76c

    invoke-virtual {v0, v1, v2, v3}, Landroid/os/Handler;->postDelayed(Ljava/lang/Runnable;J)Z

    .line 360
    return-void
.end method

.method public static showIfNeeded(Landroid/app/Activity;Ljava/lang/Runnable;)Lcom/gta/launcher/ui/IntroOverlay;
    .locals 4
    .param p0, "activity"    # Landroid/app/Activity;
    .param p1, "onFinished"    # Ljava/lang/Runnable;

    .line 85
    sget-boolean v0, Lcom/gta/launcher/ui/IntroOverlay;->shownThisProcess:Z

    const/4 v1, 0x0

    if-eqz v0, :cond_0

    return-object v1

    .line 86
    :cond_0
    const/4 v0, 0x1

    sput-boolean v0, Lcom/gta/launcher/ui/IntroOverlay;->shownThisProcess:Z

    .line 88
    :try_start_0
    new-instance v0, Lcom/gta/launcher/ui/IntroOverlay;

    invoke-direct {v0, p0, p1}, Lcom/gta/launcher/ui/IntroOverlay;-><init>(Landroid/app/Activity;Ljava/lang/Runnable;)V

    .line 89
    .local v0, "intro":Lcom/gta/launcher/ui/IntroOverlay;
    invoke-direct {v0}, Lcom/gta/launcher/ui/IntroOverlay;->build()V

    .line 90
    invoke-direct {v0}, Lcom/gta/launcher/ui/IntroOverlay;->start()V
    :try_end_0
    .catchall {:try_start_0 .. :try_end_0} :catchall_0

    .line 91
    return-object v0

    .line 92
    .end local v0    # "intro":Lcom/gta/launcher/ui/IntroOverlay;
    :catchall_0
    move-exception v0

    .line 93
    .local v0, "error":Ljava/lang/Throwable;
    const-string v2, "IntroOverlay"

    const-string v3, "Generique impossible"

    invoke-static {v2, v3, v0}, Landroid/util/Log;->e(Ljava/lang/String;Ljava/lang/String;Ljava/lang/Throwable;)I

    .line 94
    return-object v1
.end method

.method private showSlide(IIZ)V
    .locals 12
    .param p1, "drawable"    # I
    .param p2, "index"    # I
    .param p3, "finale"    # Z

    .line 290
    iget-boolean v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->finished:Z

    if-eqz v0, :cond_0

    return-void

    .line 291
    :cond_0
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->back:Landroid/widget/ImageView;

    .line 292
    .local v0, "incoming":Landroid/widget/ImageView;
    iget-object v1, p0, Lcom/gta/launcher/ui/IntroOverlay;->front:Landroid/widget/ImageView;

    iput-object v1, p0, Lcom/gta/launcher/ui/IntroOverlay;->back:Landroid/widget/ImageView;

    .line 293
    iput-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->front:Landroid/widget/ImageView;

    .line 294
    invoke-virtual {v0}, Landroid/widget/ImageView;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v1

    invoke-virtual {v1}, Landroid/view/ViewPropertyAnimator;->cancel()V

    .line 295
    iget-object v1, p0, Lcom/gta/launcher/ui/IntroOverlay;->fadeAnimator:Landroid/animation/ObjectAnimator;

    if-eqz v1, :cond_1

    iget-object v1, p0, Lcom/gta/launcher/ui/IntroOverlay;->fadeAnimator:Landroid/animation/ObjectAnimator;

    invoke-virtual {v1}, Landroid/animation/ObjectAnimator;->cancel()V

    .line 296
    :cond_1
    invoke-virtual {v0, p1}, Landroid/widget/ImageView;->setImageResource(I)V

    .line 298
    const/high16 v1, 0x40000000    # 2.0f

    invoke-virtual {v0, v1}, Landroid/widget/ImageView;->setTranslationZ(F)V

    .line 299
    iget-object v1, p0, Lcom/gta/launcher/ui/IntroOverlay;->back:Landroid/widget/ImageView;

    const/high16 v2, 0x3f800000    # 1.0f

    invoke-virtual {v1, v2}, Landroid/widget/ImageView;->setTranslationZ(F)V

    .line 301
    if-eqz p3, :cond_2

    const-wide/16 v3, 0x15e0

    goto :goto_0

    :cond_2
    const-wide/16 v3, 0x11f8

    :goto_0
    const-wide/16 v5, 0x514

    add-long/2addr v3, v5

    .line 302
    .local v3, "life":J
    if-nez p3, :cond_4

    rem-int/lit8 v1, p2, 0x2

    if-nez v1, :cond_3

    goto :goto_1

    :cond_3
    const/4 v1, 0x0

    goto :goto_2

    :cond_4
    :goto_1
    const/4 v1, 0x1

    .line 303
    .local v1, "zoomIn":Z
    :goto_2
    const/4 v7, 0x0

    if-eqz p3, :cond_5

    move v8, v7

    goto :goto_3

    :cond_5
    const/high16 v8, 0x41600000    # 14.0f

    :goto_3
    invoke-direct {p0, v8}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v8

    rem-int/lit8 v9, p2, 0x2

    if-nez v9, :cond_6

    move v9, v2

    goto :goto_4

    :cond_6
    const/high16 v9, -0x40800000    # -1.0f

    :goto_4
    mul-float/2addr v8, v9

    .line 304
    .local v8, "shift":F
    const v9, 0x3f91eb85    # 1.14f

    if-eqz v1, :cond_8

    if-eqz p3, :cond_7

    const v10, 0x3f87ae14    # 1.06f

    goto :goto_5

    :cond_7
    move v10, v9

    goto :goto_5

    :cond_8
    move v10, v2

    .line 305
    .local v10, "endScale":F
    :goto_5
    if-eqz v1, :cond_9

    move v11, v2

    goto :goto_6

    :cond_9
    move v11, v9

    :goto_6
    invoke-virtual {v0, v11}, Landroid/widget/ImageView;->setScaleX(F)V

    .line 306
    if-eqz v1, :cond_a

    goto :goto_7

    :cond_a
    move v2, v9

    :goto_7
    invoke-virtual {v0, v2}, Landroid/widget/ImageView;->setScaleY(F)V

    .line 307
    neg-float v2, v8

    invoke-virtual {v0, v2}, Landroid/widget/ImageView;->setTranslationX(F)V

    .line 308
    invoke-virtual {v0, v7}, Landroid/widget/ImageView;->setAlpha(F)V

    .line 310
    invoke-virtual {v0}, Landroid/widget/ImageView;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v2

    invoke-virtual {v2, v10}, Landroid/view/ViewPropertyAnimator;->scaleX(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v2

    invoke-virtual {v2, v10}, Landroid/view/ViewPropertyAnimator;->scaleY(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v2

    invoke-virtual {v2, v8}, Landroid/view/ViewPropertyAnimator;->translationX(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v2

    .line 311
    invoke-virtual {v2, v3, v4}, Landroid/view/ViewPropertyAnimator;->setDuration(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v2

    new-instance v7, Landroid/view/animation/LinearInterpolator;

    invoke-direct {v7}, Landroid/view/animation/LinearInterpolator;-><init>()V

    invoke-virtual {v2, v7}, Landroid/view/ViewPropertyAnimator;->setInterpolator(Landroid/animation/TimeInterpolator;)Landroid/view/ViewPropertyAnimator;

    move-result-object v2

    invoke-virtual {v2}, Landroid/view/ViewPropertyAnimator;->start()V

    .line 313
    sget-object v2, Landroid/view/View;->ALPHA:Landroid/util/Property;

    const/4 v7, 0x2

    new-array v7, v7, [F

    fill-array-data v7, :array_0

    invoke-static {v0, v2, v7}, Landroid/animation/ObjectAnimator;->ofFloat(Ljava/lang/Object;Landroid/util/Property;[F)Landroid/animation/ObjectAnimator;

    move-result-object v2

    iput-object v2, p0, Lcom/gta/launcher/ui/IntroOverlay;->fadeAnimator:Landroid/animation/ObjectAnimator;

    .line 314
    iget-object v2, p0, Lcom/gta/launcher/ui/IntroOverlay;->fadeAnimator:Landroid/animation/ObjectAnimator;

    if-nez p2, :cond_b

    if-nez p3, :cond_b

    const-wide/16 v5, 0x640

    :cond_b
    invoke-virtual {v2, v5, v6}, Landroid/animation/ObjectAnimator;->setDuration(J)Landroid/animation/ObjectAnimator;

    .line 315
    iget-object v2, p0, Lcom/gta/launcher/ui/IntroOverlay;->fadeAnimator:Landroid/animation/ObjectAnimator;

    new-instance v5, Landroid/view/animation/AccelerateDecelerateInterpolator;

    invoke-direct {v5}, Landroid/view/animation/AccelerateDecelerateInterpolator;-><init>()V

    invoke-virtual {v2, v5}, Landroid/animation/ObjectAnimator;->setInterpolator(Landroid/animation/TimeInterpolator;)V

    .line 316
    iget-object v2, p0, Lcom/gta/launcher/ui/IntroOverlay;->fadeAnimator:Landroid/animation/ObjectAnimator;

    invoke-virtual {v2}, Landroid/animation/ObjectAnimator;->start()V

    .line 318
    if-nez p2, :cond_c

    if-nez p3, :cond_c

    invoke-direct {p0}, Lcom/gta/launcher/ui/IntroOverlay;->openCinemaBars()V

    .line 319
    :cond_c
    if-nez p3, :cond_d

    invoke-direct {p0, p2}, Lcom/gta/launcher/ui/IntroOverlay;->showCaption(I)V

    .line 320
    :cond_d
    return-void

    nop

    :array_0
    .array-data 4
        0x0
        0x3f800000    # 1.0f
    .end array-data
.end method

.method private start()V
    .locals 8

    .line 263
    invoke-static {}, Ljava/lang/System;->currentTimeMillis()J

    move-result-wide v0

    iput-wide v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->startedAt:J

    .line 264
    invoke-direct {p0}, Lcom/gta/launcher/ui/IntroOverlay;->startMusic()V

    .line 266
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->progressFill:Landroid/view/View;

    invoke-virtual {v0}, Landroid/view/View;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const/high16 v1, 0x3f800000    # 1.0f

    invoke-virtual {v0, v1}, Landroid/view/ViewPropertyAnimator;->scaleX(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const-wide/16 v2, 0x5dc0

    invoke-virtual {v0, v2, v3}, Landroid/view/ViewPropertyAnimator;->setDuration(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    new-instance v2, Landroid/view/animation/LinearInterpolator;

    invoke-direct {v2}, Landroid/view/animation/LinearInterpolator;-><init>()V

    invoke-virtual {v0, v2}, Landroid/view/ViewPropertyAnimator;->setInterpolator(Landroid/animation/TimeInterpolator;)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    invoke-virtual {v0}, Landroid/view/ViewPropertyAnimator;->start()V

    .line 267
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->topMark:Landroid/view/View;

    if-eqz v0, :cond_0

    .line 268
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->topMark:Landroid/view/View;

    const/high16 v2, 0x41800000    # 16.0f

    invoke-direct {p0, v2}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v2

    neg-float v2, v2

    invoke-virtual {v0, v2}, Landroid/view/View;->setTranslationY(F)V

    .line 269
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->topMark:Landroid/view/View;

    invoke-virtual {v0}, Landroid/view/View;->animate()Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    invoke-virtual {v0, v1}, Landroid/view/ViewPropertyAnimator;->alpha(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const/4 v1, 0x0

    invoke-virtual {v0, v1}, Landroid/view/ViewPropertyAnimator;->translationY(F)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const-wide/16 v1, 0x384

    invoke-virtual {v0, v1, v2}, Landroid/view/ViewPropertyAnimator;->setStartDelay(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    const-wide/16 v1, 0x578

    invoke-virtual {v0, v1, v2}, Landroid/view/ViewPropertyAnimator;->setDuration(J)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    new-instance v1, Landroid/view/animation/AccelerateDecelerateInterpolator;

    invoke-direct {v1}, Landroid/view/animation/AccelerateDecelerateInterpolator;-><init>()V

    .line 270
    invoke-virtual {v0, v1}, Landroid/view/ViewPropertyAnimator;->setInterpolator(Landroid/animation/TimeInterpolator;)Landroid/view/ViewPropertyAnimator;

    move-result-object v0

    invoke-virtual {v0}, Landroid/view/ViewPropertyAnimator;->start()V

    .line 273
    :cond_0
    const/4 v0, 0x0

    .local v0, "pass":I
    :goto_0
    const/4 v1, 0x4

    if-ge v0, v1, :cond_1

    .line 274
    move v1, v0

    .line 275
    .local v1, "index":I
    iget-object v2, p0, Lcom/gta/launcher/ui/IntroOverlay;->handler:Landroid/os/Handler;

    new-instance v3, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda0;

    invoke-direct {v3, p0, v1}, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda0;-><init>(Lcom/gta/launcher/ui/IntroOverlay;I)V

    int-to-long v4, v0

    const-wide/16 v6, 0x11f8

    mul-long/2addr v4, v6

    invoke-virtual {v2, v3, v4, v5}, Landroid/os/Handler;->postDelayed(Ljava/lang/Runnable;J)Z

    .line 273
    .end local v1    # "index":I
    add-int/lit8 v0, v0, 0x1

    goto :goto_0

    .line 277
    .end local v0    # "pass":I
    :cond_1
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->handler:Landroid/os/Handler;

    new-instance v1, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda1;

    invoke-direct {v1, p0}, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda1;-><init>(Lcom/gta/launcher/ui/IntroOverlay;)V

    const-wide/16 v2, 0x47e0

    invoke-virtual {v0, v1, v2, v3}, Landroid/os/Handler;->postDelayed(Ljava/lang/Runnable;J)Z

    .line 279
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->handler:Landroid/os/Handler;

    new-instance v1, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda2;

    invoke-direct {v1, p0}, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda2;-><init>(Lcom/gta/launcher/ui/IntroOverlay;)V

    const-wide/16 v2, 0x2ee0

    invoke-virtual {v0, v1, v2, v3}, Landroid/os/Handler;->postDelayed(Ljava/lang/Runnable;J)Z

    .line 285
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->handler:Landroid/os/Handler;

    new-instance v1, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda3;

    invoke-direct {v1, p0}, Lcom/gta/launcher/ui/IntroOverlay$$ExternalSyntheticLambda3;-><init>(Lcom/gta/launcher/ui/IntroOverlay;)V

    const-wide/16 v2, 0x5848

    invoke-virtual {v0, v1, v2, v3}, Landroid/os/Handler;->postDelayed(Ljava/lang/Runnable;J)Z

    .line 286
    return-void
.end method

.method private startMusic()V
    .locals 3

    .line 364
    :try_start_0
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    sget v1, Lcom/gta/game/R$raw;->crestwood_intro:I

    invoke-static {v0, v1}, Landroid/media/MediaPlayer;->create(Landroid/content/Context;I)Landroid/media/MediaPlayer;

    move-result-object v0

    iput-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->player:Landroid/media/MediaPlayer;

    .line 365
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->player:Landroid/media/MediaPlayer;

    if-nez v0, :cond_0

    return-void

    .line 366
    :cond_0
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->player:Landroid/media/MediaPlayer;

    const v1, 0x3f666666    # 0.9f

    invoke-virtual {v0, v1, v1}, Landroid/media/MediaPlayer;->setVolume(FF)V

    .line 367
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->player:Landroid/media/MediaPlayer;

    const/4 v1, 0x1

    invoke-virtual {v0, v1}, Landroid/media/MediaPlayer;->setLooping(Z)V

    .line 369
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->player:Landroid/media/MediaPlayer;

    invoke-virtual {v0}, Landroid/media/MediaPlayer;->start()V
    :try_end_0
    .catchall {:try_start_0 .. :try_end_0} :catchall_0

    .line 373
    goto :goto_0

    .line 370
    :catchall_0
    move-exception v0

    .line 371
    .local v0, "error":Ljava/lang/Throwable;
    const-string v1, "IntroOverlay"

    const-string v2, "Musique du generique indisponible"

    invoke-static {v1, v2, v0}, Landroid/util/Log;->e(Ljava/lang/String;Ljava/lang/String;Ljava/lang/Throwable;)I

    .line 372
    invoke-direct {p0}, Lcom/gta/launcher/ui/IntroOverlay;->releasePlayer()V

    .line 374
    .end local v0    # "error":Ljava/lang/Throwable;
    :goto_0
    return-void
.end method

.method private text(Ljava/lang/String;FIZF)Landroid/widget/TextView;
    .locals 6
    .param p1, "value"    # Ljava/lang/String;
    .param p2, "sp"    # F
    .param p3, "color"    # I
    .param p4, "bold"    # Z
    .param p5, "spacing"    # F

    .line 122
    new-instance v0, Landroid/widget/TextView;

    iget-object v1, p0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    invoke-direct {v0, v1}, Landroid/widget/TextView;-><init>(Landroid/content/Context;)V

    .line 123
    .local v0, "view":Landroid/widget/TextView;
    invoke-virtual {v0, p1}, Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V

    .line 124
    invoke-virtual {v0, p3}, Landroid/widget/TextView;->setTextColor(I)V

    .line 125
    const/4 v1, 0x2

    invoke-virtual {v0, v1, p2}, Landroid/widget/TextView;->setTextSize(IF)V

    .line 126
    iget-object v1, p0, Lcom/gta/launcher/ui/IntroOverlay;->activity:Landroid/app/Activity;

    if-eqz p4, :cond_0

    invoke-static {v1}, Lcom/gta/game/ui/CwUi;->black(Landroid/content/Context;)Landroid/graphics/Typeface;

    move-result-object v1

    goto :goto_0

    :cond_0
    invoke-static {v1}, Lcom/gta/game/ui/CwUi;->medium(Landroid/content/Context;)Landroid/graphics/Typeface;

    move-result-object v1

    :goto_0
    invoke-virtual {v0, v1}, Landroid/widget/TextView;->setTypeface(Landroid/graphics/Typeface;)V

    .line 127
    invoke-virtual {v0, p5}, Landroid/widget/TextView;->setLetterSpacing(F)V

    .line 128
    const/high16 v1, 0x40c00000    # 6.0f

    invoke-direct {p0, v1}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v1

    const/high16 v2, 0x3f800000    # 1.0f

    invoke-direct {p0, v2}, Lcom/gta/launcher/ui/IntroOverlay;->dp(F)F

    move-result v2

    const/16 v3, 0xdc

    const/4 v4, 0x0

    invoke-static {v3, v4, v4, v4}, Landroid/graphics/Color;->argb(IIII)I

    move-result v3

    const/4 v5, 0x0

    invoke-virtual {v0, v1, v5, v2, v3}, Landroid/widget/TextView;->setShadowLayer(FFFI)V

    .line 129
    invoke-virtual {v0, v4}, Landroid/widget/TextView;->setIncludeFontPadding(Z)V

    .line 130
    return-object v0
.end method


# virtual methods
.method public pause()V
    .locals 1

    .line 421
    :try_start_0
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->player:Landroid/media/MediaPlayer;

    if-eqz v0, :cond_0

    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->player:Landroid/media/MediaPlayer;

    invoke-virtual {v0}, Landroid/media/MediaPlayer;->isPlaying()Z

    move-result v0

    if-eqz v0, :cond_0

    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->player:Landroid/media/MediaPlayer;

    invoke-virtual {v0}, Landroid/media/MediaPlayer;->pause()V
    :try_end_0
    .catchall {:try_start_0 .. :try_end_0} :catchall_0

    goto :goto_0

    :catchall_0
    move-exception v0

    .line 422
    :cond_0
    :goto_0
    return-void
.end method

.method public release()V
    .locals 2

    .line 429
    iget-boolean v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->finished:Z

    if-eqz v0, :cond_0

    return-void

    .line 430
    :cond_0
    const/4 v0, 0x1

    iput-boolean v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->finished:Z

    .line 431
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->handler:Landroid/os/Handler;

    const/4 v1, 0x0

    invoke-virtual {v0, v1}, Landroid/os/Handler;->removeCallbacksAndMessages(Ljava/lang/Object;)V

    .line 432
    invoke-direct {p0}, Lcom/gta/launcher/ui/IntroOverlay;->releasePlayer()V

    .line 433
    return-void
.end method

.method public resume()V
    .locals 1

    .line 425
    :try_start_0
    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->player:Landroid/media/MediaPlayer;

    if-eqz v0, :cond_0

    iget-boolean v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->finished:Z

    if-nez v0, :cond_0

    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->player:Landroid/media/MediaPlayer;

    invoke-virtual {v0}, Landroid/media/MediaPlayer;->isPlaying()Z

    move-result v0

    if-nez v0, :cond_0

    iget-object v0, p0, Lcom/gta/launcher/ui/IntroOverlay;->player:Landroid/media/MediaPlayer;

    invoke-virtual {v0}, Landroid/media/MediaPlayer;->start()V
    :try_end_0
    .catchall {:try_start_0 .. :try_end_0} :catchall_0

    goto :goto_0

    :catchall_0
    move-exception v0

    .line 426
    :cond_0
    :goto_0
    return-void
.end method
