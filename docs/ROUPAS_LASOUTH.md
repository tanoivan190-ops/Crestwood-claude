# Roupas Lasouth no Crestwood

Integração local com 1.223 peças: camisas, calças, calçados, mochilas, máscaras, brincos e relógios. Os assets vieram de `C:\LASouth\atualizacao\Realized\arquivos\root\root\cache-full\custom`, aberto somente para leitura. Nenhum script executa builds ou grava arquivos no Lasouth.

A interface foi refeita em HTML/CSS/JavaScript, em português, em `app/src/main/assets/clothes/`. Ela usa a ponte `AndroidClothes` do WebView já instalado no projeto, nas duas variantes Android. A biblioteca CEF nativa deste projeto existe apenas em 32 bits; a interface de roupas pelo WebView funciona também em ARM64.

## Uso

Abra a loja de roupas existente, use `/vetements` ou o botão de roupas do HUD. A aba **Coleção** mostra as peças importadas. **Ajustes originais** mantém cabelo, rosto, tatuagens e o catálogo anterior. Giro, aproximação e foco por categoria são executados no thread do jogo. Descartar restaura o visual de entrada; salvar envia o visual ao servidor.

Os modelos masculino e feminino são carregados uma vez por corpo. DFF/TXD são extraídos apenas para o cache privado do aplicativo. As texturas PNG/JPEG ficam no APK e são decodificadas sob demanda; o cache tenta liberar prévias sem referências ao ultrapassar 64 MiB e preserva texturas em uso por jogadores.

## Persistência e compatibilidade

O servidor deste checkout só guardava 25 campos, embora o cliente já enviasse 39. Agora guarda 46: os 39 anteriores mais sete IDs de peças. Registros antigos com 25 ou 39 campos são migrados na leitura, sem apagar o banco. O valor zero nos campos novos conserva o catálogo anterior. IDs seguem a ordem exportada do catálogo; não devem ser reordenados depois de haver visuais salvos.

`server/basicgm/basicgm1.pwn` foi mantido em Latin-1. O `.amx` correspondente foi recompilado com o compilador Windows copiado para `tools/pawn-windows/`; seu formato é AMX versão 8. O compilador reporta um aviso de recursão em `Admin_OnCommand`, sem erros.

## Verificações

- `tools/check_clothes_native.ps1`: verificação sintática com o NDK 26.2 dos cinco arquivos nativos envolvidos, em ARM64 e ARM de 32 bits.
- `node tools/verify_clothes_ui.cjs`: ações da interface, comandos da ponte, seleção e restauração, acessórios, câmera, ajustes originais e layout em 960×540, 800×360 e 1280×720.
- `tools/verify_south_import.py`: hashes da origem e dos assets copiados, limites dos IDs, ordem dos 46 campos, sincronização dos clientes e cabeçalho AMX.
- Capturas da interface em `out/clothes-validation/`.

Ainda é necessário gerar/instalar o APK e validar no Android: carregamento RenderWare, aparência das peças no personagem, oclusão, cancelamento, reconexão e visual visto por outro jogador. A verificação do NDK é sintática; não substitui build/link do APK ou teste no dispositivo. Não foi feito deploy de servidor ou APK.

## Reimportação

`tools/import_south_clothes.py <pasta-custom>` importa o catálogo e recusa mudança na associação dos IDs já exportados. `tools/import_south_models.py` documenta a origem dos modelos e a adaptação inicial; não é um comando de build. Os hashes de origem ficam em `app/src/main/assets/clothes/south/source-hashes.json`.
