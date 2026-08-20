from typing import List, Union, Iterable, Optional, Container
import numpy as np
from matplotlib import pyplot as plt
from .abstract import AnalysisResults

PLOT_COLORS = ["r", "g", "b", "orange", "magenta", "cyan", "yellow", "lime", "violet", "chocolate", "slategrey", "lavender"] # 12 colors. First 4 match cwipc_tilecolor().

DEFAULT_PLOT_STYLE = ["count","cumulative"]

def set_default_plot_style(style : Union[str, Iterable[str]]):
    global DEFAULT_PLOT_STYLE
    if isinstance(style, str):
        DEFAULT_PLOT_STYLE = style.split(',')
    else:
        DEFAULT_PLOT_STYLE = list(style)
        

class Plotter:
    #: Title for the plot (usually determined by this class)
    title : str
    results : List[AnalysisResults]

    def __init__(self, title : str):
        self.title = title
        pass

    def set_results(self, results : List[AnalysisResults]) -> None:
        """Set the results to be plotted. This will clear the current plot."""
        self.results = results

    def plot(self, filename : Optional[str]=None, show : bool = False, which : Optional[Container[str]]=None):
        assert self.results
        # xxxjack This uses the stateful pyplot API. Horrible.
        if not filename and not show:
            return
        if which is None:
            which = DEFAULT_PLOT_STYLE
        do_count = which is None or 'count' in which or 'all' in which
        do_cumulative = which is None or 'cumulative' in which or 'all' in which
        do_delta = which is None or 'delta' in which or 'all' in which
        do_log = which is not None and 'log' in which
        do_log_cumulative = False # do_log
        nCamera = len(self.results)
        plot_fig, plot_ax = plt.subplots()
        if do_log:
            plot_ax.set_yscale('symlog')
        plot_ax.set_xlabel("Distance (m)")
        plot_ax.set_ylabel(do_log and "log(count)" or "count")
        ax_cum = None
        if do_cumulative:
            ax_cum = plot_ax.twinx()
            if do_log_cumulative:
                ax_cum.set_yscale('log')
            ax_cum.set_ylabel(do_log_cumulative and "log(cumulative)" or "cumulative")
        corr_box_text = "Correspondence:\n"
        corr_box_dict = {}
        assert self.results
        variant = None
        algorithm = None
        for cam_i in range(nCamera):
            results = self.results[cam_i]
            cam_tilenum = results.tilemask
            ref_tilenum = results.referenceTilemask
            histogram = results.histogram
            histogramEdges = results.histogramEdges
            corr = results.minCorrespondence
            variant = results.variant # Assumes they are all the same
            algorithm = results.algorithm   # Assumes they are all the same
            label = f"{cam_tilenum}"
            if ref_tilenum:
                label += f" vs {ref_tilenum}"
            corr_box_text += f"\n{label}: {results.tostr()}"
            corr_box_dict[label] = results.tostr()
            assert histogram is not None
            assert histogramEdges is not None
            #(histogram, edges, cumsum, normsum, plot_label, raw_distances) = h_data
            plot_ax.plot(histogramEdges[1:], histogram, label=label, color=PLOT_COLORS[cam_i % len(PLOT_COLORS)])
            
            if do_cumulative:
                assert ax_cum
                cumsum = np.cumsum(histogram)
                totDistances = cumsum[-1]
                normsum = cumsum / totDistances
                ax_cum.plot(histogramEdges[1:], normsum, linestyle="dashed", label="_nolegend_", color=PLOT_COLORS[cam_i % len(PLOT_COLORS)])
                ax_cum.plot([corr, corr], [0, 1], linestyle="dotted",  label="_nolegend_", color=PLOT_COLORS[cam_i % len(PLOT_COLORS)])
                # xxxjack should also plot mean, mode, etc.
            if do_delta:
                # Compute deltas over intervals of half of "corr" size
                
                corr_bin = int(np.digitize(corr, histogramEdges))
                nbin = len(histogram) // (corr_bin//2)
                while len(histogram) % nbin != 0:
                    nbin += 1
                new_edges = histogramEdges[0::nbin]
                new_histo = np.reshape(histogram, (-1, nbin)).sum(axis=1)/nbin
                delta = np.diff(new_histo)
                plot_ax.plot([new_edges[0], new_edges[-1]], [0, 0], linestyle="solid", label="_nolegend_", color="black", linewidth=0.2)
                plot_ax.plot(new_edges[1:-1], delta, marker=".", linewidth=0, label="_nolegend_", color=PLOT_COLORS[cam_i % len(PLOT_COLORS)])
        title = self.title
        if algorithm:
            title = f"{title}\n{algorithm}"
        if variant:
            title = f"{title} ({variant})"
        plt.title(title)
        props = dict(boxstyle='round', facecolor='white', alpha=0.5)
        #plot_ax.text(0.98, 0.1, corr_box_text, transform=plot_ax.transAxes, fontsize='small', verticalalignment='bottom', horizontalalignment="right", bbox=props)
        handles, labels = plot_ax.get_legend_handles_labels()
        labels = [x + ": " + corr_box_dict.get(x, "") for x in labels]
        plot_fig.subplots_adjust(bottom=0.2)
        plot_fig.legend(handles, labels, loc="lower center", bbox_to_anchor=(0.5, 0.0))
        if filename:
            plt.savefig(filename)
        if show:
            plt.show()
            plt.close()
    