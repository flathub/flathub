# Mosca Tee

Editor gráfico profissional, gratuito e acessível para Linux.

## Sobre

Mosca Tee é um editor gráfico inovador focado em **privacidade**, **processamento local** e **acessibilidade avançada** para pessoas cegas e de baixa visão.

## Acessibilidade

🦟 **Mosca Tee é diferente**: Desenvolvido com foco especial em usuários com deficiência visual.

### Recursos de Acessibilidade:
- **Navegação por Teclado Completa** - Todas as funções acessíveis sem mouse
- **Leitores de Tela Compatíveis** - NVDA, JAWS e outros leitores funcionam nativamente
- **Descrições de Áudio** - Interface com feedback auditivo detalhado
- **Contraste Alto** - Modo de alto contraste para baixa visão
- **Ampliação de Interface** - Suporte a zoom da interface
- **Ferramentas Adaptadas** - Lasso, seleção mágica e outras com suporte a acessibilidade
- **Sem Dependência Visual** - Edição completa sem necessidade de visão

### Como Usar com Leitor de Tela:
1. Execute: `flatpak run com.moscatee.Editor`
2. Ative seu leitor de tela (NVDA, JAWS, etc)
3. Use Tab/Shift+Tab para navegar
4. Use Enter/Espaço para ativar funções
5. Teclas de atalho estão disponíveis em português

**Contribuições de acessibilidade são bem-vindas!** Se encontrar qualquer barreira, reporte em: https://github.com/moscatee/editor/issues

---

## Informações

- **App ID**: com.moscatee.Editor
- **Homepage**: https://moscatee.com
- **License**: GPL-3.0+

## Compilação

```bash
flatpak-builder --user --install --force-clean build-dir com.moscatee.Editor.yaml
```

## Executar

```bash
flatpak run com.moscatee.Editor
```
