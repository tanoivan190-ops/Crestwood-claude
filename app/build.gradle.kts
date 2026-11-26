// [v52] imports explicites : dans ce script, "java." designe l'extension Gradle "java", pas le package
//       (erreur GitHub Actions "Unresolved reference: security / io").
import java.io.ByteArrayOutputStream
import java.io.File
import java.security.MessageDigest
import java.security.SecureRandom
import javax.crypto.Cipher
import javax.crypto.spec.IvParameterSpec
import javax.crypto.spec.SecretKeySpec

plugins {
    id("com.android.application")
}

// [v50] FIREBASE SANS PLUGIN google-services : on lit app/google-services.json et on genere les
//       ressources que Firebase attend (google_app_id, google_api_key, project_id,
//       default_web_client_id...). Le client du package de l'appli est prefere ; sinon le premier.
//       Le client OAuth "Web" (default_web_client_id) n'existe qu'une fois le fournisseur Google
//       active dans la console Firebase (Authentication > Sign-in method > Google).
//       Forcer une valeur : -Pcrestwood.googleWebClientId=xxxx.apps.googleusercontent.com
fun cwFirebaseConfig(f: File, pkg: String): Map<String, String> {
    val out = linkedMapOf(
        "google_app_id" to "", "google_api_key" to "", "project_id" to "",
        "gcm_defaultSenderId" to "", "google_storage_bucket" to "", "default_web_client_id" to ""
    )
    if (!f.isFile) return out
    val root = groovy.json.JsonSlurper().parse(f) as? Map<*, *> ?: return out
    val info = root["project_info"] as? Map<*, *>
    out["project_id"] = info?.get("project_id")?.toString() ?: ""
    out["gcm_defaultSenderId"] = info?.get("project_number")?.toString() ?: ""
    out["google_storage_bucket"] = info?.get("storage_bucket")?.toString() ?: ""
    val clients = (root["client"] as? List<*>)?.filterIsInstance<Map<*, *>>() ?: emptyList()
    fun pkgOf(c: Map<*, *>): String? =
        ((c["client_info"] as? Map<*, *>)?.get("android_client_info") as? Map<*, *>)?.get("package_name")?.toString()
    val c = clients.firstOrNull { pkgOf(it) == pkg } ?: clients.firstOrNull()
    if (c != null) {
        out["google_app_id"] = (c["client_info"] as? Map<*, *>)?.get("mobilesdk_app_id")?.toString() ?: ""
        out["google_api_key"] = ((c["api_key"] as? List<*>)?.firstOrNull() as? Map<*, *>)?.get("current_key")?.toString() ?: ""
    }
    val oauth = mutableListOf<Map<*, *>>()
    clients.forEach { cl ->
        (cl["oauth_client"] as? List<*>)?.filterIsInstance<Map<*, *>>()?.let { oauth.addAll(it) }
        val inv = (cl["services"] as? Map<*, *>)?.get("appinvite_service") as? Map<*, *>
        (inv?.get("other_platform_oauth_client") as? List<*>)?.filterIsInstance<Map<*, *>>()?.let { oauth.addAll(it) }
    }
    out["default_web_client_id"] = oauth.firstOrNull { it["client_type"]?.toString() == "3" }?.get("client_id")?.toString() ?: ""
    return out
}
val cwFirebase = cwFirebaseConfig(file("google-services.json"), "com.crestwoodroleplay").toMutableMap().also { m ->
    (findProperty("crestwood.googleWebClientId") as String?)?.takeIf { it.isNotBlank() }?.let { m["default_web_client_id"] = it.trim() }
}
// [v52] projet Firebase de Crestwood : crestwoodrp-3e53d. Rappel au build si le fichier n'est pas encore le bon.
if (cwFirebase["project_id"] != "crestwoodrp-3e53d") {
    logger.warn("[crestwood] google-services.json = projet '${cwFirebase["project_id"]}' : remplace-le par celui de crestwoodrp-3e53d (appli com.crestwoodroleplay)")
}
if (cwFirebase["default_web_client_id"].isNullOrEmpty()) {
    logger.warn("[crestwood] aucun client Web OAuth dans google-services.json : active Google dans Authentication puis retelecharge le fichier")
}

android {
    compileSdk = 36

    namespace = "com.gta.game"

    defaultConfig {
        applicationId = "com.crestwoodroleplay"   // [v54] = appli Android du projet Firebase crestwoodrp-3e53d
        minSdk = 28
        targetSdk = 36
        versionCode = 4
        versionName = "2.11-crestwood"

        multiDexEnabled = true

        ndk {
            abiFilters.add("arm64-v8a")
            abiFilters.add("armeabi-v7a")
        }

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        // [v50] configuration Firebase (connexion Google) : voir cwFirebaseConfig plus haut
        cwFirebase.forEach { (k, v) -> resValue("string", k, v) }
    }

    // [v50] SIGNATURE FIXE (debug ET release) : sans elle, chaque build GitHub Actions signe avec une
    //       nouvelle cle de debug -> l'empreinte SHA-1 change et Google refuse la connexion.
    //       Empreintes a declarer dans Firebase (Parametres du projet > appli Android com.crestwoodroleplay) :
    //         SHA-1   FF:62:E8:37:A6:AF:89:C3:89:1C:CE:14:8D:5A:CB:4F:5C:00:43:92
    //         SHA-256 F0:2D:60:AB:22:9E:C0:C3:BD:B1:53:CE:8A:A7:15:67:DF:F2:83:E1:5A:59:1F:30:2E:F5:C3:71:4C:86:D3:F4
    signingConfigs {
        create("crestwood") {
            storeFile = file("crestwood-debug.jks")
            storePassword = "crestwood50"
            keyAlias = "crestwood"
            keyPassword = "crestwood50"
        }
    }

    packaging {
        jniLibs {
            excludes.add("META-INF/*")
        }
        resources {
            excludes.add("META-INF/*")
        }
    }

    ndkVersion = "26.2.11394342"

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
        }
    }

    applicationVariants.all {
        outputs.all {
            val apkName = "CrestwoodRP-${buildType.name}.apk"
            (this as com.android.build.gradle.internal.api.BaseVariantOutputImpl).outputFileName = apkName
        }
    }

    buildTypes {
        debug {
            signingConfig = signingConfigs.getByName("crestwood")
            isDebuggable = true
            isJniDebuggable = true
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            ndk {
                debugSymbolLevel = "FULL"
            }
        }
        release {
            signingConfig = signingConfigs.getByName("crestwood")
            isDebuggable = false
            isJniDebuggable = false
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            ndk {
                debugSymbolLevel = "FULL"
            }
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    lint {
        checkReleaseBuilds = false
        abortOnError = false
    }

    buildFeatures {
        prefab = true
        resValues = true   // [v50] ressources Firebase generees
    }
}

dependencies {
    implementation(fileTree(mapOf("dir" to "libs", "include" to listOf("*.jar"))))
    implementation(files("libs/sampmobilecef-1.0.0-release.aar"))
    implementation("androidx.appcompat:appcompat:1.6.1")
    implementation("com.google.android.material:material:1.9.0")
    implementation("androidx.constraintlayout:constraintlayout:2.1.4")
    implementation("androidx.navigation:navigation-fragment:2.7.2")
    implementation("androidx.navigation:navigation-ui:2.7.2")
    implementation("androidx.core:core-ktx:1.12.0")
    implementation("com.google.android.play:asset-delivery:2.3.0")
    testImplementation("junit:junit:4.13.2")
    androidTestImplementation("androidx.test.ext:junit:1.1.5")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.5.1")
    implementation("com.android.volley:volley:1.2.1")
    implementation("com.intuit.sdp:sdp-android:1.1.0")
    implementation("androidx.datastore:datastore-preferences:1.0.0")
    implementation("androidx.datastore:datastore-core:1.0.0")
    implementation("org.ini4j:ini4j:0.5.4")
    implementation("com.github.bumptech.glide:glide:4.15.1")
    annotationProcessor("com.github.bumptech.glide:compiler:4.15.1")
    implementation("androidx.lifecycle:lifecycle-process:2.6.2")
    implementation("com.joom.paranoid:paranoid-gradle-plugin:0.3.14")
    implementation("com.bytedance.android:shadowhook:1.0.10")
    implementation("androidx.multidex:multidex:2.0.1")
    implementation("com.github.amitshekhariitbhu:PRDownloader:1.0.2")
    // [v50] connexion Google -> Firebase Authentication (com.crestwood.ui.CwGoogleAuth)
    implementation(platform("com.google.firebase:firebase-bom:33.7.0"))
    implementation("com.google.firebase:firebase-auth")
    implementation("androidx.credentials:credentials:1.3.0")
    implementation("androidx.credentials:credentials-play-services-auth:1.3.0")
    implementation("com.google.android.libraries.identity.googleid:googleid:1.1.1")
}

// [v47] PAGES PROTEGEES : les .html .js .css .json de NOS dossiers d'assets sont chiffres dans l'APK
//       ("CWX1" + IV 16 octets + AES/CTR). L'appli les dechiffre a la volee : com.crestwood.ui.CwAssets.
//       Desactiver : ./gradlew assembleDebug -Pcrestwood.protectPages=false
fun cwEncryptAssets(dir: File, roots: Set<String>): Int {
    val seed = "Crest" + "wood" + ":" + "pa" + "ges" + ":" + "v47" + ":" + "a9F3k" + "Q7zLm2X"
    val key = MessageDigest.getInstance("SHA-256").digest(seed.toByteArray(Charsets.UTF_8)).copyOf(16)
    val ks = SecretKeySpec(key, "AES")
    val rnd = SecureRandom()
    val exts = setOf("html", "htm", "js", "css", "json")
    var n = 0
    dir.walkTopDown().filter { it.isFile }.forEach { f ->
        val rel = f.relativeTo(dir).invariantSeparatorsPath
        val top = rel.substringBefore('/')
        if (!roots.contains(top) || !exts.contains(f.extension.lowercase())) return@forEach
        val raw = f.readBytes()
        if (raw.size >= 4 && raw[0] == 'C'.code.toByte() && raw[1] == 'W'.code.toByte() && raw[2] == 'X'.code.toByte() && raw[3] == '1'.code.toByte()) return@forEach
        val iv = ByteArray(16)
        rnd.nextBytes(iv)
        val c = Cipher.getInstance("AES/CTR/NoPadding")
        c.init(Cipher.ENCRYPT_MODE, ks, IvParameterSpec(iv))
        val out = ByteArrayOutputStream(raw.size + 20)
        out.write(byteArrayOf(0x43, 0x57, 0x58, 0x31))
        out.write(iv)
        out.write(c.doFinal(raw))
        f.writeBytes(out.toByteArray())
        n++
    }
    return n
}
val cwProtectPages = (project.findProperty("crestwood.protectPages")?.toString() ?: "true").toBoolean()
val cwAssetRoots: Set<String> = (file("src/main/assets").listFiles() ?: arrayOf()).filter { it.isDirectory }.map { it.name }.toSet()
tasks.configureEach {
    val tn = name
    if (cwProtectPages && tn.startsWith("merge") && tn.endsWith("Assets") && !tn.contains("Shader") && !tn.contains("Jni")) {
        doLast {
            var total = 0
            outputs.files.files.filter { it.isDirectory }.forEach { d -> total += cwEncryptAssets(d, cwAssetRoots) }
            logger.lifecycle("[crestwood] pages protegees : $total fichier(s) chiffre(s) ($tn)")
        }
    }
}
