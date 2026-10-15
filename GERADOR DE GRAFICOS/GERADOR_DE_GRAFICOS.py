import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os

# ============================================================
# CONFIGURAÇÕES
# ============================================================

# Arquivos que serão analisados
arquivos = [
    "DADOS DE ANALISE BANCADA/ON-OFF.txt",
    "DADOS DE ANALISE BANCADA/PI.txt",
    "DADOS DE ANALISE BANCADA/PID.txt",
]

# Umidade relativa desejada (%)
umidade_desejada = 50.0

# ============================================================
# CONFIGURAÇÃO DO GRÁFICO
# ============================================================

fig, ax = plt.subplots(figsize=(12, 6))

cores = [
    "darkgrey",
    "darkblue",
    "gold"
]


# ============================================================
# LEITURA E PLOTAGEM DOS ARQUIVOS
# ============================================================

for i, arquivo in enumerate(arquivos):

    # Leitura do arquivo
    tabela = pd.read_csv(
        arquivo,
        encoding="utf-8",
        sep=","
    )

    # Seleciona somente Tempo e H2
    tabela_analise = tabela[["Tempo", "H2"]].dropna()

    tempo = tabela_analise["Tempo"]
    H2 = tabela_analise["H2"]

    # ========================================================
    # MÉDIA E DESVIO PADRÃO
    # ========================================================

    media_H2 = np.mean(H2)
    desvio_H2 = np.std(H2, ddof=1)

    # Nome do arquivo sem caminho e sem .txt
    nome_arquivo = os.path.splitext(
        os.path.basename(arquivo)
    )[0]

    # ========================================================
    # PLOTAGEM
    # ========================================================

    ax.plot(
        tempo,
        H2,

        # Cor do respectivo ensaio
        color=cores[i],

        # Pontos
        marker='o',
        markersize=4,

        # Linha ligando os pontos
        linestyle='-',
        linewidth=1.5,

        # Borda dos pontos
        markeredgecolor='black',

        # Legenda
        label=(
            f'{nome_arquivo} '
            f'(μ={media_H2:.2f} %, '
            f'σ={desvio_H2:.2f} %)'
        )
    )


# ============================================================
# LINHA DE UMIDADE DESEJADA
# ============================================================

ax.axhline(
    y=umidade_desejada,
    color='black',
    linestyle='--',
    linewidth=2,
    label=f'Setpoint = {umidade_desejada:.1f} %'
)


# ============================================================
# CONFIGURAÇÕES DO GRÁFICO
# ============================================================


ax.set_xlabel(
    "Tempo (s)",
    fontsize=26,
    fontweight='bold',
    labelpad=10
)

ax.set_ylabel(
    "Umidade Relativa H2 (%)",
    fontsize=26,
    fontweight='bold',
    labelpad=10
)

ax.set_ylim(0, 70)

ax.set_yticks(np.arange(0, 70, 10))

ax.tick_params(
    axis='both',
    which='major',
    labelsize=25
)

ax.grid(
    True,
    linestyle=':',
    alpha=0.6
)

ax.legend(
    loc="lower right",
    fontsize=23,
    frameon=True,
    framealpha=0.9
)

plt.tight_layout()

plt.show()